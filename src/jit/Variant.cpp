#include "jit/runtime/Variant.hpp"
#include "jit/CodeCompiler.hpp"
#include <utility>

Variant::Variant(CompiledCCodePtr code, ProfilingDataManager *p, JitRuntime *runtime)
    : code(std::move(code)), runtime(runtime) {
  this->profilingDataManager = p;
  this->valid = true;
  this->activeThreads = 0;

  this->startime = std::chrono::system_clock::now();
}

// pipeline function declaration
typedef uint32_t (*OpenPtr)(GlobalState *, Dispatcher *, Variant *);
typedef uint32_t (*InitPtr)(GlobalState *, Dispatcher *);
typedef uint32_t (*ExecutePtr)(int, int);
typedef uint32_t (*MigrateFrom)(void **);
typedef uint32_t (*MigrateTo)(void **);
typedef void **(*GetStatePtr)();

void Variant::init(GlobalState *globalState, Dispatcher *dispatcher) {
  auto initFunction = (*code->getFunctionPointer<InitPtr>("_Z4initP11GlobalStateP10Dispatcher"));
  initFunction(globalState, dispatcher);
}

void Variant::open(GlobalState *globalState, Dispatcher *dispatcher) {
  auto openFunction = (*code->getFunctionPointer<OpenPtr>("_Z4openP11GlobalStateP10DispatcherP7Variant"));
  openFunction(globalState, dispatcher, this);
}

void Variant::migrateTo(void **outputState) {
  auto migrateFromFunction = (*code->getFunctionPointer<MigrateTo>("_Z9migrateToPPv"));
  migrateFromFunction(outputState);
}

void Variant::migrateFrom(void **inputState) {
  auto migrateFromFunction = (*code->getFunctionPointer<MigrateFrom>("_Z11migrateFromPPv"));
  migrateFromFunction(inputState);
}

void Variant::execute(int threadID, int numaNode) {
  auto executeFromFunction = (*code->getFunctionPointer<ExecutePtr>("_Z7executeii"));
  executeFromFunction(threadID, numaNode);
}

void **Variant::getState() {
  auto getStateFunction = (*code->getFunctionPointer<GetStatePtr>("_Z8getStatev"));
  return getStateFunction();
}
#include <jit/JITCodeGenerator.h>

#include <jit/CodeCompiler.hpp>
#include <operator/InputOperator.h>

#include "jit/JITExecutionRuntime.h"
#include <chrono>
#include <jit/runtime/JitDispatcher.h>
#include <jit/runtime/SimpleDispatcher.h>

JitRuntime::JitRuntime() {}

JITExecutionRuntime::JITExecutionRuntime() {
  currentState = DEFAULT;
  this->basename = std::to_string(std::rand());
  this->running = true;
}

bool JITExecutionRuntime::isRunning() { return running; }

Variant *JITExecutionRuntime::compileVariant(Query *query, ProfilingDataManager *profilingDataManager,
                                             CompileMode mode) {

  JITCodeGenerator codeGenerator = JITCodeGenerator(query->config, query->schema, profilingDataManager, mode);
  QueryContext queryContext = QueryContext(query->schema);
  codeGenerator.addQueryContext(queryContext);
  query->current->produce(codeGenerator);
  Operator *input = query->root;

  while (input->rightChild) {
    input = input->rightChild;
  }

  InputOperator *inputOp = (InputOperator *)input;
  codeGenerator.generateStructFile(inputOp->getPath());
  auto file = codeGenerator.generate(inputOp->getInputTypeAsString(), inputOp->getPath());

  CCodeCompiler codeCompiler = CCodeCompiler();
  const CompiledCCodePtr &code = codeCompiler.compile(
      file.output, this->basename + "_" + std::to_string((this->variantNr++)) + "_" + std::to_string(mode));

  return new Variant(code, codeGenerator.profilingDataManager, this);
}

void JITExecutionRuntime::runWorker(JITExecutionRuntime *runtime, int threadId) {

  while (runtime->isRunning()) {

    std::cout << "Thread: " << threadId << " start executing pipeline." << std::endl;
    Variant *currentVariant = runtime->currentlyExecutingVariant;

    currentVariant->activeThreads++;
    try {
      currentVariant->execute(threadId, 0);
    } catch (DeoptimizeException &e) {
      std::cerr << "Thread: " << threadId << " deoptimize: in pipeline: " << e.pipeline
                << " buffer position: " << e.position << std::endl;

      {
        std::unique_lock<std::mutex> lk(runtime->waitMutex);
        if (runtime->currentState == OPTIMIZED) {
          runtime->compileCondition.notify_all();
        }
        runtime->compilationFinish.wait(lk, [runtime] { return runtime->currentState == DEFAULT; });
        std::cout << "Thread: " << threadId << " restarts again" << std::endl;
      }
    }

    std::cout << "Thread: " << threadId << " returned from variant" << std::endl;
    auto oldVariant = currentVariant;
    // check if current thread is the last who left the variant
    auto oldValue = oldVariant->activeThreads.fetch_sub(1) - 1;
    if (oldValue == 0) {
      std::cout << "Thread: " << threadId << " returned last. So he has to migrate" << std::endl;
      if (runtime->currentState == OPTIMIZED) {
        void **state = oldVariant->getState();
        runtime->currentlyExecutingVariant->migrateFrom(state);
      } else {
        void **state = runtime->currentlyExecutingVariant->getState();
        oldVariant->migrateTo(state);
      }
    }
  }
}

void JITExecutionRuntime::compilationLoop(JITExecutionRuntime *jitExecutionRuntime) {
  int runs = 0;
  while (runs < 10) {
    std::this_thread::sleep_for(std::chrono::milliseconds(jitExecutionRuntime->delay));
    if (jitExecutionRuntime->currentState == DEFAULT) {
      std::cerr << "------------- Deploy Profiling Code NOW ---------- " << std::endl;
      jitExecutionRuntime->deployInstrumented();

    } else if (jitExecutionRuntime->currentState == INSTRUMENTED) {

      std::cerr << "------------- Deploy Optimized Code NOW ---------- " << std::endl;
      jitExecutionRuntime->deployOptimized();

      {
        std::cerr << "-------------Main thread Waits ---------- " << std::endl;
        std::unique_lock<std::mutex> lk(jitExecutionRuntime->compilationMutex);
        jitExecutionRuntime->compileCondition.wait(lk);
        std::cerr << "-------------Main thread has to deoptimize ---------- " << std::endl;
        jitExecutionRuntime->deployInstrumented();
        jitExecutionRuntime->compilationFinish.notify_all();
      }
    }

    runs++;
  }
}

void JITExecutionRuntime::monitor(int threadID) {}

void JITExecutionRuntime::execute(Query *query) {
  std::string papi_conf_file = "papi_conf_global.cfg";
  std::string config = "Branch_Benchmark";
  delay = query->config.getCompilationDelay();
  dispatcher =
      new SimpleDispatcher(query->config.getRunLength(), query->config.getParallelism(), query->config.getBufferSize(),
                           1, query->schema.getInputSize(), query->config.getSourceFile(), 0);
  globalState = new GlobalState();
  globalState->window_state = new WindowState *[5];

  this->query = query;
  this->currentState = DEFAULT;
  auto profilingDataManager = new ProfilingDataManager();
  std::cerr << "------------- Deploy Default Code NOW ---------- " << std::endl;
  auto variant = compileVariant(query, nullptr, CM_DEFAULT);
  variant->open(globalState, dispatcher);

  variant->init(globalState, dispatcher);

  defaultVariant = variant;

  currentlyExecutingVariant = variant;
  /* Launch a group of threads. */
  auto *t = new std::thread[dispatcher->parallelism];
  for (size_t i = 0; i < dispatcher->parallelism; i++) {
    t[i] = std::thread(JITExecutionRuntime::runWorker, this, i);
  }

  auto compilationThread = new std::thread(JITExecutionRuntime::compilationLoop, this);

  std::this_thread::sleep_for(std::chrono::seconds(query->config.getBenchmarkRunDuration()));
  this->running = false;

  /* Wait for all threads. */
  for (size_t i = 0; i < dispatcher->parallelism; i++) {
    t[i].join();
  }
  compilationThread->join();
}

void JITExecutionRuntime::deployOptimized() {
  // In the state machine we only deploy OPTIMIZED when we are INSTRUMENTED
  assert(this->currentState == INSTRUMENTED);
  auto newVariant = compileVariant(query, this->currentlyExecutingVariant->profilingDataManager, CM_OPTIMIZE);
  newVariant->open(globalState, dispatcher);
  std::cout << "we optimized to the default code" << std::endl;
  auto oldVariant = currentlyExecutingVariant;
  currentlyExecutingVariant = newVariant;
  this->currentState = OPTIMIZED;
  oldVariant->invalidate();
}

void JITExecutionRuntime::deployInstrumented() {
  // In the state machine we only deploy instrumented when we are Default
  // assert(this->currentState == DEFAULT);
  auto profilingDataManager = new ProfilingDataManager();
  auto newVariant = compileVariant(query, profilingDataManager, CM_INSTRUMENT);
  newVariant->open(globalState, dispatcher);
  std::cout << "we instrumented the default code" << std::endl;
  auto oldVariant = currentlyExecutingVariant;
  currentlyExecutingVariant = newVariant;
  this->currentState = INSTRUMENTED;
  oldVariant->invalidate();
}

void JITExecutionRuntime::deployDefault() {
  // In the state machine we only deploy Default when we are optimized
  assert(this->currentState == OPTIMIZED);
  auto newVariant = compileVariant(query, nullptr, CM_DEFAULT);
  newVariant->open(globalState, dispatcher);
  std::cout << "we deoptimized to the default code" << std::endl;
  auto oldVariant = currentlyExecutingVariant;
  currentlyExecutingVariant = newVariant;
  this->currentState = DEFAULT;

  oldVariant->invalidate();
}

void JITExecutionRuntime::deoptimize(Variant *currentVariant, void *buffer, int position) {
  // we can only deoptimize when we are currently optimized
  assert(this->currentState == OPTIMIZED);
  redeploy.lock();
  if (currentVariant == currentlyExecutingVariant) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    // if (currentState == DEFAULT)
    //    JitExecutionRuntime::deployInstrumented();
    // else if (currentState == INSTRUMENTED)
    //   JitExecutionRuntime::deployOptimized();
    // else if (currentState == OPTIMIZED)
    JITExecutionRuntime::deployDefault();
    // invalidate current variant
    currentVariant->invalidate();

  } else {
    std::cout << "the pipeline was already redeployed" << std::endl;
  }
  redeploy.unlock();
}

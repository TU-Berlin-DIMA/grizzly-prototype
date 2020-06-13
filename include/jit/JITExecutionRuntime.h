#ifndef RUNTIME_JIT_H
#define RUNTIME_JIT_H

#include "api/Query.h"
#include "condition_variable"
#include "mutex"
#include "runtime/JitRuntime.h"
#include <jit/runtime/Variant.hpp>

enum PipelineState { DEFAULT, INSTRUMENTED, OPTIMIZED };

class JITExecutionRuntime : JitRuntime {
public:
  JITExecutionRuntime();
  void deoptimize(Variant *variant, void *buffer, int position);
  void execute(Query *query);
  bool isRunning();
  void monitor(int threadID) override;

private:
  Variant *currentlyExecutingVariant;
  Variant *defaultVariant;
  static void runWorker(JITExecutionRuntime *runtime, int threadID);
  void deployDefault();
  std::mutex redeploy;
  int delay;
  Query *query;
  GlobalState *globalState;
  Dispatcher *dispatcher;
  std::atomic<PipelineState> currentState;
  int variantNr;
  std::string basename;
  std::atomic_bool running;

  void deployInstrumented();

  void deployOptimized();

  Variant *compileVariant(Query *query, ProfilingDataManager *profilingDataManager, CompileMode mode);
  std::condition_variable compileCondition;
  std::condition_variable compilationFinish;
  std::mutex compilationMutex;
  std::mutex waitMutex;

  static void compilationLoop(JITExecutionRuntime *jitExecutionRuntime);
  static void monitor(JITExecutionRuntime *jitExecutionRuntime);
};

#endif
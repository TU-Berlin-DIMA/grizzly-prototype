#ifndef GRIZZLY_VARIANT_HPP
#define GRIZZLY_VARIANT_HPP

#include "JitDispatcher.h"
#include "JitRuntime.h"
#include "Profiling.h"
#include "jit_global_state.hpp"
#include <atomic>
#include <chrono>
#include <numeric>

class CompiledCCode;
typedef std::shared_ptr<CompiledCCode> CompiledCCodePtr;

/**
 * @brief A variant encapsulates a compiled query.
 * Each pipeline can consist of multiple pipeline steps and utility methods.
 */
class Variant {

public:
  Variant(CompiledCCodePtr code, ProfilingDataManager *profilingDataManager, JitRuntime *runtime);

  /**
   * @brief Opens the new variant. This is called by each worker thread
   * @param globalState
   * @param dispatcher
   */
  void open(GlobalState *globalState, Dispatcher *dispatcher);

  /**
   * @brief Initializes the new variant. This is called by the runtime once per compiled varient.
   * @param globalState
   * @param dispatcher
   */
  void init(GlobalState *globalState, Dispatcher *dispatcher);

  /**
   * @brief This executes the variant, with a particular worker thread on a particular numa node.
   * @param threadID
   * @param numaNode
   */
  void execute(int threadID, int numaNode);

  /**
   * @brief migrates from another variant to this variant.
   * @param inputState
   */
  void migrateFrom(void **inputState);

  /**
   * @brief migrates from this variant to another variant
   * @param outputState
   */
  void migrateTo(void **outputState);

  void **getState();

  /**
   * @brief Indicates if this variant is still valid.
   * @return
   */
  bool isValid() { return valid; };

  /**
   * @brief Invalidates this variant.
   */
  void invalidate() { valid = false; }

  // the number of threads currently executing this variant.
  std::atomic_int activeThreads;

  JitRuntime *runtime;

  ProfilingDataManager *profilingDataManager;
  std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>> startime;

private:
  CompiledCCodePtr code;
  std::atomic_bool valid;
};

#endif // GRIZZLY_VARIANT_HPP

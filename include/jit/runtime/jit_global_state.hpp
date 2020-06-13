#include "tbb/atomic.h"
#ifndef GRIZZLY_JIT_GLOBAL_STATE_HPP
#define GRIZZLY_JIT_GLOBAL_STATE_HPP

/**
 * Thread local state struct, to keep track of current window accross pipeline invocations.
 */
struct ThreadLocalState {
  u_int64_t current_window = 0;
  int64_t *windowEnds;
};

/**
 * Global struct to keep track of window state
 */
struct WindowState {
  // Counter for how many threads had a local triggered.
  tbb::atomic<u_int32_t> global_tigger_counter = 0;
  // Array of thread local state
  ThreadLocalState **thread_local_state;
};

struct GlobalState {
  /**
   * Stores window metadata across all pipelines
   */
  WindowState **window_state;
};

#endif // GRIZZLY_JIT_GLOBAL_STATE_HPP

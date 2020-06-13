#ifndef RUNTIME_RUNTIME_H
#define RUNTIME_RUNTIME_H

#include <atomic>
#include <csignal>
#include <iostream>
#include <tbb/atomic.h>
#include <tbb/concurrent_unordered_map.h>
#include <thread>
#include <vector>

#include "JitDispatcher.h"

/**
 * @brief This exception represents the deoptimization from an optimized query back to an unoptimized state
 */
struct DeoptimizeException : public std::exception {
  int position;
  int pipeline;
  void *buffer;
  DeoptimizeException(int pipeline, int position, void *buffer)
      : position(position), pipeline(pipeline), buffer(buffer) {}
};

class JitRuntime {
public:
  JitRuntime();
  virtual void monitor(int threadID){};
};

#endif // RUNTIME_RUNTIME_H

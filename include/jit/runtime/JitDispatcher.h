#ifndef RUNTIME_DISPATCHER_H
#define RUNTIME_DISPATCHER_H

#include "runtime/input_types.h"
#include <cassert>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <numa.h>
#include <tbb/atomic.h>
#include <thread>

static inline int fast_atoi(const char *str) {
  int val = 0;
  while (*str) {
    val = (val << 4) - (val << 2) - (val << 1) + (*str++ - '0');
  }
  return val;
}

class Dispatcher {
public:
  Dispatcher(unsigned int runLength, unsigned int parallelism, unsigned int bufferSize, unsigned int runs,
             unsigned int tupleSize, std::string file, int numa)
      : runLength(runLength), parallelism(parallelism), bufferSize(bufferSize), runs(runs), tupleSize(tupleSize),
        numa(numa), file(file) {

    buffer = new void **[parallelism];
    bufferRuns = bufferSize / runLength;
    std::cout << "bufferSize" << bufferSize << " runLength" << runLength << " run buffers " << bufferRuns << std::endl;
    std::string papi_conf_file = "papi_conf_global.cfg";
    std::string config = "Branch_Preset";
  }

  virtual void loadData() = 0;

  virtual void tick(long second) = 0;

  int seconds = 0;
  static void throughputLogger(Dispatcher *dispatcher) {

    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto lastOutput = std::chrono::system_clock::now();
    std::ofstream file;
    file.open("throughput.csv", std::ios::out | std::ios::app);
    while (dispatcher->seconds < 500) {

      auto current = std::chrono::system_clock::now();
      auto div = current - lastOutput;
      if (div >= std::chrono::seconds(1)) {
        auto processed = dispatcher->buffersProcessed.fetch_and_store(0);
        auto tuple = (processed * dispatcher->runLength);
        auto milliSec = std::chrono::duration_cast<std::chrono::milliseconds>(div).count();
        double thorughput = (((double)tuple) / (double)milliSec) * 1000;
        std::cout << "Buffer Processed: " << tuple << " in: " << milliSec << "ms"
                  << " is " << thorughput << " tps" << std::endl;
        file << dispatcher->seconds << ";" << ((int64_t)thorughput) << "\n";
        file.flush();
        dispatcher->seconds++;
        dispatcher->tick(dispatcher->seconds);
        lastOutput = current;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    file.close();
  }

  void *getWork(int thread_id, int run) {
    buffersProcessed++;
    void *t = buffer[thread_id][run];
    return t;
  }

  bool hasWork() {
    // TODO: add if file is empty
    // TODO: add if file was read entirely
    return true;
  }

  void stop() { stopped = true; }

  void cleanup() {
    for (unsigned int i = 0; i < parallelism; i++) {
      // delete[] buffer[i];
    }
  }

  unsigned int runLength;
  unsigned int bufferRuns;
  unsigned int tupleSize;
  unsigned int bufferSize;
  unsigned int parallelism;
  unsigned int numa;
  int *numa_relation;
  std::string file;
  tbb::atomic<size_t> buffersProcessed = 0;

protected:
  void readBinaryFileIntoBuffer(size_t thread_id) {
    std::ifstream ifp(path.c_str(), std::ios::in | std::ios::binary);
    assert(ifp.is_open());
    ifp.read(reinterpret_cast<char *>(buffer[thread_id]), bufferSize * tupleSize);
    ifp.close();
  }

  unsigned int runs;
  bool stopped;

  void ***buffer;
  std::string path;
  InputType type;
};

#endif // RUNTIME_DISPATCHER_H

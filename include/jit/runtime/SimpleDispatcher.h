#include "jit/runtime/JitDispatcher.h"

class SimpleDispatcher : public Dispatcher {
public:
  SimpleDispatcher(unsigned int runLength, unsigned int parallelism, unsigned int bufferSize, unsigned int runs,
                   unsigned int tupleSize, std::string file, int numa)
      : Dispatcher(runLength, parallelism, bufferSize, runs, tupleSize, file, numa) {
    loadData();
    std::thread t = std::thread(Dispatcher::throughputLogger, this);
    t.detach();
  }

  void loadData() {
    std::cout << "Load Data from " << file << std::endl;
    buffer = new void **[parallelism];
    for (size_t thread = 0; thread < parallelism; thread++) {
      buffer[thread] = new void *[bufferRuns];
      for (size_t bufferRun = 0; bufferRun < bufferRuns; bufferRun++) {
        buffer[thread][bufferRun] = malloc(tupleSize * runLength);
        std::ifstream ifp(file, std::ios::in | std::ios::binary);
        assert(ifp.is_open());
        ifp.read(reinterpret_cast<char *>(buffer[thread][bufferRun]), runLength * tupleSize);
        ifp.close();
      }
    }
  }

  void tick(long s) {}
};
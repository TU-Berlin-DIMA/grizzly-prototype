#ifndef API_CONFIG_H
#define API_CONFIG_H

#include <string>

class Config {
public:
  static Config create();

  /**
   * @brief Configures the number of worker threads, which process the input.
   * @param parallelism number of worker threads
   * @return
   */
  Config &withParallelism(unsigned int parallelism);
  unsigned int getParallelism();

  /**
   * @brief Configures the size of the input buffer.
   * @param bufferSize
   * @return
   */
  Config &withBufferSize(unsigned int bufferSize);
  unsigned int getBufferSize();

  /**
   * @brief Configures the runLength -> the number of records processed at once
   * @param bufferSize
   * @return
   */
  Config &withRunLength(unsigned int runLength);
  unsigned int getRunLength();

  unsigned int getPipelinePermutation();
  Config &withPipelinePermutation(unsigned int pipelinePermuation);

  /**
   * @brief Configure the duration of an benchmark run.
   * @param runDuration
   * @return
   */
  Config &withBenchmarkRunDuration(unsigned int runDuration);
  unsigned int getBenchmarkRunDuration();

  /**
   * @brief Configure the delay before the jit compiler switches to the next compilation stage.
   * @param delay in ms
   * @return
   */
  Config &withCompilationDelay(unsigned int delay);
  unsigned int getCompilationDelay();

  Config &withOutputBuffer(unsigned size);
  unsigned int getOutputBuffer();
  bool filterOpt();
  bool distributionOpt();
  Config &withDistributionOpt(bool disOpt);
  Config &withFilterOpt(bool filterOpt);

  Config &withNuma(bool numa);
  bool getNuma();
  int getNumaNodes();

  const std::string &getSourceFile() const;
  void setSourceFile(const std::string &sourceFile);

private:
  Config();
  unsigned int parallelism;
  unsigned int runLength;
  unsigned int bufferSize;
  unsigned int pipelinePermutation;
  unsigned int benchmarkRunTime;
  unsigned int compilationDelay;
  unsigned int outputBuffer;
  bool numa;
  bool filterOptimizations;
  bool distributionOptimizations;
  std::string sourceFile;
};

#endif // API_CONFIG_H

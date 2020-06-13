#include "api/Config.h"
#include <numa.h>

Config::Config() {
  parallelism = 1;
  bufferSize = 1;
  runLength = 1;
  pipelinePermutation = 0;
  benchmarkRunTime = 60;
  numa = false;
  filterOptimizations = true;
  distributionOptimizations = true;
}

Config Config::create() { return Config(); }

Config &Config::withBufferSize(unsigned int bufferSize) {
  this->bufferSize = bufferSize;
  return *this;
}

bool Config::filterOpt() { return filterOptimizations; }

Config &Config::withFilterOpt(bool filterOpt) {
  filterOptimizations = filterOpt;
  return *this;
}

Config &Config::withParallelism(unsigned int parallelism) {
  this->parallelism = parallelism;
  return *this;
}

Config &Config::withBenchmarkRunDuration(unsigned int runDuration) {
  this->benchmarkRunTime = runDuration;
  return *this;
}

Config &Config::withPipelinePermutation(unsigned int pipelinePermutation) {
  this->pipelinePermutation = pipelinePermutation;
  return *this;
}

Config &Config::withOutputBuffer(unsigned int size) {
  this->outputBuffer = size;
  return *this;
}

unsigned int Config::getOutputBuffer() { return this->outputBuffer; }

bool Config::getNuma() { return numa; }
int Config::getNumaNodes() {
  if (numa)
    return numa_num_configured_nodes();
  return 1;
}
Config &Config::withNuma(bool numa) {
  this->numa = numa;
  return *this;
}

unsigned int Config::getBenchmarkRunDuration() { return benchmarkRunTime; }

Config & Config::withCompilationDelay(unsigned int delay) {
  compilationDelay = delay;
  return *this;
}

unsigned int Config::getCompilationDelay() {
  return compilationDelay;
}

unsigned int Config::getBufferSize() { return bufferSize; }

unsigned int Config::getParallelism() { return parallelism; }

unsigned int Config::getPipelinePermutation() { return pipelinePermutation; }

unsigned int Config::getRunLength() { return runLength; }

Config &Config::withRunLength(unsigned int runLength) {
  this->runLength = runLength;
  return *this;
}

Config &Config::withDistributionOpt(bool disOpt) { this->distributionOptimizations = disOpt; return *this;}

bool Config::distributionOpt() { return distributionOptimizations; }
const std::string &Config::getSourceFile() const { return sourceFile; }
void Config::setSourceFile(const std::string &sourceFile) { Config::sourceFile = sourceFile; }

//
// Created by pgrulich on 18.09.19.
//

#include "jit/runtime/Profiling.h"

ProfilingHandler::ProfilingHandler(){};

MinProfilingHandler::MinProfilingHandler() : ProfilingHandler() { value = INT32_MAX; }

MaxProfilingHandler::MaxProfilingHandler() : ProfilingHandler() { value = -1; }

ProfilingDataManager::ProfilingDataManager() {}

void ProfilingDataManager::registerMinHandler(std::string handlerName) {
  this->handlers[handlerName] = (ProfilingHandler *)new MinProfilingHandler();
}

void ProfilingDataManager::registerMaxHandler(std::string name) {
  handlers[name] = (ProfilingHandler *)new MaxProfilingHandler();
}

void ProfilingDataManager::registerSelectivityHandler(std::string name, unsigned long i) {
  handlers[name] = (ProfilingHandler *)new SelectivityHandler(i);
}

void ProfilingDataManager::registerDistributionHandler(std::string name) {
  handlers[name] = (ProfilingHandler *)new DistributionProfilingHandler();
}
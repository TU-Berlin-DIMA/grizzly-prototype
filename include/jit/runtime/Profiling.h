#ifndef GRIZZLY_PROFILING_H
#define GRIZZLY_PROFILING_H

#include "atomic"
#include "string"
#include "tbb/concurrent_unordered_map.h"
#include "vector"
#include <algorithm> // std::find
#include <vector>

class ProfilingHandler {
public:
  ProfilingHandler();
};

class MinProfilingHandler : ProfilingHandler {
public:
  MinProfilingHandler();

  void update(int64_t i) {
    if (i < value)
      value = i;
  }

  int getValue() { return value; }

private:
  std::atomic_int value;
};

class DistributionProfilingHandler : ProfilingHandler {
public:
  DistributionProfilingHandler() {
    k = 10;
    top = std::vector<int>(k + 1);
    for (int i = 0; i < k + 1; i++) {
      this->top[i] = 0;
    }
  }

  std::vector<int> top;
  tbb::concurrent_unordered_map<int, int> freq;
  int k;

  void update(int value) {

    // increase the frequency
    freq[value]++;
    top[k] = value;

    auto it = std::find(top.begin(), top.end() - 1, value);
    for (int i = distance(top.begin(), it) - 1; i >= 0; --i) {
      // compare the frequency and swap if higher
      // frequency element is stored next to it
      if (freq[top[i]] < freq[top[i + 1]])
        std::swap(top[i], top[i + 1]);

      // if frequency is same compare the elements
      // and swap if next element is high
      else if ((freq[top[i]] == freq[top[i + 1]]) && (top[i] > top[i + 1]))
        std::swap(top[i], top[i + 1]);
      else
        break;
    }
  }

  int getValue() { return value; }

private:
  std::atomic_int value;
};

class SelectivityHandler : ProfilingHandler {
public:
  SelectivityHandler(unsigned long s) : ProfilingHandler() {
    this->values = new std::atomic_uint_fast64_t[s];
    this->counter = 0;
    for (int i = 0; i < s; i++) {
      this->values[i] = 0;
    }
  };

  void update(int64_t predicate, int64_t outcome) { this->values[predicate] += outcome; }

  int operator++() { return counter++; }

  std::atomic_uint_fast64_t *getValue() { return this->values; }

  std::atomic_uint_fast64_t counter;

private:
  std::atomic_uint_fast64_t *values;
};

class MaxProfilingHandler : ProfilingHandler {
public:
  MaxProfilingHandler();

  void update(int64_t i) {
    if (i > value)
      value = i;
  }

  int getValue() { return value; }

private:
  std::atomic_int value;
};

class ProfilingDataManager {
public:
  ProfilingDataManager();

  ProfilingHandler *getHandler(std::string handlerName) { return handlers[handlerName]; }

  MaxProfilingHandler *getMaxHandler(std::string name) { return (MaxProfilingHandler *)getHandler(name); }

  MinProfilingHandler *getMinHandler(std::string name) { return (MinProfilingHandler *)getHandler(name); }

  DistributionProfilingHandler *getDistributionProfilingHandler(std::string name) {
    return (DistributionProfilingHandler *)getHandler(name);
  }

  SelectivityHandler *getSelectivityHandler(std::string name) { return (SelectivityHandler *)getHandler(name); }

  void registerMinHandler(std::string name);

  void registerMaxHandler(std::string name);

  void registerSelectivityHandler(std::string name, unsigned long i);

  void registerDistributionHandler(std::string name);

private:
  tbb::concurrent_unordered_map<std::string, ProfilingHandler *> handlers;
};

#endif // GRIZZLY_PROFILING_H

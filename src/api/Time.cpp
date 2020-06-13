#include "api/Time.h"

Time Time::seconds(size_t seconds) {
  Time time(seconds);
  return time;
}

Time Time::minutes(size_t minutes) {
  Time time(minutes * 60);
  return time;
}

std::string Time::to_string() { return std::to_string(this->time); }
#ifndef API_WINDOW_H
#define API_WINDOW_H

#include <string>

#include "api/Assigner.h"
#include "api/Time.h"
#include "api/Trigger.h"
#include "operator/Operator.h"

class Counter {
public:
  Counter(size_t max) : max(max) {}

  size_t max;

private:
};

class Window {
public:
  Assigner *assigner;
  Trigger *trigger;

  Window withTrigger(Trigger &&trigger) {
    this->trigger = &trigger;
    return *this;
  }
};

class TumblingProcessingTimeWindow : public Window {
public:
  TumblingProcessingTimeWindow(Time size) {
    assigner = new TumblingProcessingTimeAssigner(size);
    trigger = new PurgingTrigger(new ProcessingTimeTrigger(size));
  }

  TumblingProcessingTimeWindow(Counter size) {
    assigner = new TumblingProcessingTimeAssigner(Time::seconds(size.max));
    trigger = new PurgingTrigger(new CountTrigger(size.max));
  }
};

class SlidingProcessingTimeWindow : public Window {
public:
  SlidingProcessingTimeWindow(Time size, Time slide) {
    assigner = new SlidingProcessingTimeAssigner(size, slide);
    trigger = new PurgingTrigger(new ProcessingTimeTrigger(slide));
  }
};

class SessionProcessingTimeWindow : public Window {
public:
  SessionProcessingTimeWindow(Time timeout) {
    assigner = new SessionProcessingTimeAssigner(timeout);
    trigger = new PurgingTrigger(
        new ProcessingTimeTrigger(Time::seconds(365 * 24 * 60 * 60))); // maximum session timeout of one year
  }
};


#endif // API_WINDOW_H

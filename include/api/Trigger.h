#ifndef API_TRIGGER_H
#define API_TRIGGER_H

#include "api/Time.h"
#include "code_generation/CodeGenerator.h"

class Trigger {

public:
  virtual void onBeforeElement(CodeGenerator &cg, size_t pipeline) = 0;
  virtual void onBeforeAssign(CodeGenerator &cg, size_t pipeline) = 0;
  bool purge = false;
  virtual std::string to_string() { return "trigger"; };
};

class CountTrigger : public Trigger {
public:
  CountTrigger(size_t maxCount) : maxCount(maxCount) {}

  void onBeforeElement(CodeGenerator &cg, size_t pipeline) override;
  void onBeforeAssign(CodeGenerator &cg, size_t pipeline) override{};
  std::string to_string() override { return "CountTrigger"; }

private:
  size_t maxCount;
};

class ProcessingTimeTrigger : public Trigger {
public:
  ProcessingTimeTrigger(Time every) : every(every) {}

  void onBeforeElement(CodeGenerator &cg, size_t pipeline) override{};
  void onBeforeAssign(CodeGenerator &cg, size_t pipeline) override;
  std::string to_string() override { return "ProcessingTimeTrigger"; }

private:
  Time every;
};



class PurgingTrigger : public Trigger {
public:
  PurgingTrigger(Trigger *trigger) : trigger(trigger) {}
  PurgingTrigger(Trigger &&trigger) : trigger(&trigger) {}

  void onBeforeElement(CodeGenerator &cg, size_t pipeline) override;
  void onBeforeAssign(CodeGenerator &cg, size_t pipeline) override;
  std::string to_string() override { return "PurgingTrigger"; }

private:
  Trigger *trigger;
};

#endif // APPI_TRIGGER_H

#ifndef OPERATOR_WINDOW_OPERATOR_H
#define OPERATOR_WINDOW_OPERATOR_H

#include "api/Assigner.h"
#include "api/Trigger.h"
#include "operator/Operator.h"

class WindowOperator : public Operator {
public:
  WindowOperator(Assigner *assigner, Trigger *trigger, Operator *input);
  ~WindowOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  Assigner *assigner;
  Trigger *trigger;
  Operator *input;
};

#endif // OPERATOR_WINDOW_OPERATOR_H

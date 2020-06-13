#ifndef OPERATOR_KEY_OPERATOR_H
#define OPERATOR_KEY_OPERATOR_H

#include "operator/Operator.h"

class KeyOperator : public Operator {
public:
  KeyOperator(Field &field, Operator *input);
  ~KeyOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  Field &field;
  Operator *input;
};

#endif // OPERATOR_KEY_OPERATOR_H

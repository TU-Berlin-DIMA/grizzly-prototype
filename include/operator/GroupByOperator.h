#ifndef OPERATOR_GROUPBY_OPERATOR_H
#define OPERATOR_GROUPBY_OPERATOR_H

#include "operator/Operator.h"

class GroupByOperator : public Operator {
public:
  GroupByOperator(Field &field, Operator *input);
  GroupByOperator(Field &field, Operator *input, int maxValue);
  ~GroupByOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  Field &field;
  Operator *input;
  int maxValue;
};

#endif // OPERATOR_GROUPBY_OPERATOR_H

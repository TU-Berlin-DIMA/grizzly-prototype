#ifndef OPERATOR_PRINT_OPERATOR_H
#define OPERATOR_PRINT_OPERATOR_H

#include "operator/Operator.h"

class PrintOperator : public Operator {
public:
  PrintOperator(Operator *input);
  ~PrintOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  Operator *input;
};

#endif // OPERATOR_PRINT_OPERATOR_H

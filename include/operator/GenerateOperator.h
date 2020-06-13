#ifndef OPERATOR_GENERATE_OPERATOR_H
#define OPERATOR_GENERATE_OPERATOR_H

#include "operator/Operator.h"

class GenerateOperator : public Operator {
public:
  GenerateOperator(Operator *input);
  ~GenerateOperator();
  void consume(CodeGenerator &cg) override;
  void produce(CodeGenerator &cg) override;
  std::string to_string() override;

private:
  Operator *input;
};

#endif // OPERATOR_GENERATE_OPERATOR_H

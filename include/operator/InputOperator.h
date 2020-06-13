#ifndef OPERATOR_INPUT_OPERATOR_H
#define OPERATOR_INPUT_OPERATOR_H

#include "operator/Operator.h"

class InputOperator : public Operator {
public:
  InputOperator(InputType type, std::string path, Operator *input);
  ~InputOperator();
  void consume(CodeGenerator &cg) override;
  void produce(CodeGenerator &cg) override;
  std::string to_string() override;
  InputType getInputType();
  std::string getInputTypeAsString();
  std::string getPath();

private:
  InputType type;
  std::string path;
  Operator *input;
};

#endif // OPERATOR_INPUT_OPERATOR_H

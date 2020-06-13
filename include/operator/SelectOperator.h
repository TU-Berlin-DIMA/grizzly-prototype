#ifndef OPERATOR_SELECT_OPERATOR_H
#define OPERATOR_SELECT_OPERATOR_H

#include "operator/Operator.h"

class SelectOperator : public Operator {
public:
  SelectOperator(Operator *input, std::vector<std::string> fields);
  ~SelectOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  Operator *input;
  std::vector<std::string> fields;
};

#endif // OPERATOR_SELECT_OPERATOR_H

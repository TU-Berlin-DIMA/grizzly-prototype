#ifndef OPERATOR_WRITE_OPERATOR_H
#define OPERATOR_WRITE_OPERATOR_H

#include "operator/Operator.h"

class WriteOperator : public Operator {
public:
  WriteOperator(std::string fileName, Operator *input);
  ~WriteOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  std::string fileName;
  Operator *input;
};

#endif // OPERATOR_WRITE_OPERATOR_H

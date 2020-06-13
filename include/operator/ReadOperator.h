#ifndef OPERATOR_READ_OPERATOR_H
#define OPERATOR_READ_OPERATOR_H

#include "operator/Operator.h"

class ReadOperator : public Operator {
public:
  ReadOperator(Schema &schema);
  ~ReadOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  Schema &schema;
  Operator *input;
};

#endif // OPERATOR_READ_OPERATOR_H

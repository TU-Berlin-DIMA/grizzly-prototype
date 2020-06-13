#ifndef OPERATOR_READ_WINDOW_OPERATOR_H
#define OPERATOR_READ_WINDOW_OPERATOR_H

#include "operator/Operator.h"

class ReadWindowOperator : public Operator {
public:
  ReadWindowOperator(Schema &schema, Operator *input);
  ~ReadWindowOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  Schema &schema;
  Operator *input;
};

#endif // OPERATOR_READ_OPERATOR_H

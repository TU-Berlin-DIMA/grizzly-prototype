#ifndef OPERATOR_FinalWindowAggOperator_OPERATOR_H
#define OPERATOR_FinalWindowAggOperator_OPERATOR_H

#include "api/Aggregation.h"
#include "api/Mapper.h"
#include "operator/Operator.h"

class FinalWindowAggOperator : public Operator {
public:
  FinalWindowAggOperator(Aggregation *aggregation, Operator *input);
  ~FinalWindowAggOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  Operator *input;
  Aggregation *aggregation;
};

#endif // OPERATOR_FinalWindowAggOperator_OPERATOR_H

#ifndef OPERATOR_AGGREGATE_OPERATOR_H
#define OPERATOR_AGGREGATE_OPERATOR_H

#include "api/Aggregation.h"
#include "operator/Operator.h"

class AggregateOperator : public Operator {
public:
  AggregateOperator(Aggregation &aggregation, Operator *input);
  ~AggregateOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  Aggregation &aggregation;
  Operator *input;
};

#endif // OPERATOR_AGGREGATE_OPERATOR_H

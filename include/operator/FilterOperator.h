#ifndef OPERATOR_FILTER_OPERATOR_H
#define OPERATOR_FILTER_OPERATOR_H

#include "api/Predicate.h"
#include "operator/Operator.h"

class FilterOperator : public Operator {
public:
  FilterOperator(Predicate &predicate, Operator *input);
  ~FilterOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  Predicate &predicate;
  Operator *input;
};

#endif // OPERATOR_FILTER_OPERATOR_H

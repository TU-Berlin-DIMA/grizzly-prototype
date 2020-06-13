#include <sstream>
#include <string>

#include "operator/AggregateOperator.h"

AggregateOperator::AggregateOperator(Aggregation &aggregation, Operator *input)
    : aggregation(aggregation), input(input) {
  leftChild = NULL;
  rightChild = NULL;
  input->parent = this;
  name = "Aggregate";
}

std::string AggregateOperator::to_string() { return "Aggregate " + aggregation.to_string(); }

AggregateOperator::~AggregateOperator() { delete input; }

void AggregateOperator::consume(CodeGenerator &cg) {
  // delegate to specific aggregation function
  aggregation.consume(cg, parent);
}

void AggregateOperator::produce(CodeGenerator &cg) {
  // delegate to specific aggregation function
  aggregation.produce(cg, input);
}

#include <sstream>
#include <string>

#include "operator/FinalWindowAggOperator.h"

FinalWindowAggOperator::FinalWindowAggOperator(Aggregation *aggregation, Operator *input)
    : aggregation(aggregation), input(input) {

  leftChild = NULL;
  rightChild = NULL;
  input->parent = this;
  name = "Map";
}

std::string FinalWindowAggOperator::to_string() { return "FinalWindowAggregation"; }

FinalWindowAggOperator::~FinalWindowAggOperator() { delete input; }

void FinalWindowAggOperator::consume(CodeGenerator &cg) {
  // delegate to specific mapper function
  aggregation->consumeFinalAggregation(cg, parent);
  parent->consume(cg);
}

void FinalWindowAggOperator::produce(CodeGenerator &cg) {
  // delegate to specific mapper function
  aggregation->produceFinalAggregation(cg, input);
  input->produce(cg);
}

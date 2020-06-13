#include <sstream>
#include <string>

#include "operator/FilterOperator.h"

FilterOperator::FilterOperator(Predicate &predicate, Operator *input) : predicate(predicate), input(input) {
  leftChild = NULL;
  rightChild = NULL;
  input->parent = this;
  name = "Select";
}

std::string FilterOperator::to_string() { return "Select "; }

FilterOperator::~FilterOperator() { delete input; }

void FilterOperator::consume(CodeGenerator &cg) {
  // delegate to specific predicate function

  predicate.consume(cg, parent);
}

void FilterOperator::produce(CodeGenerator &cg) {
  // delegate to specific predicate function
  predicate.produce(cg, input);
}

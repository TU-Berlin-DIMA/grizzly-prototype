#include <sstream>
#include <string>

#include "operator/InputOperator.h"

InputOperator::InputOperator(InputType pType, std::string pPath, Operator *input)
    : type(pType), path(pPath), input(input) {
  leftChild = NULL;
  rightChild = NULL;
  input->parent = this;
  name = "Input";
}

InputOperator::~InputOperator() { delete input; }

std::string InputOperator::to_string() { return "Input"; }

void InputOperator::consume(CodeGenerator &cg) {
  // delegate to specific predicate function
  if (parent != nullptr) {
    parent->consume(cg);
  }
  // predicate.consume(cg, parent);
}

void InputOperator::produce(CodeGenerator &cg) {
  // delegate to specific predicate function
  // predicate.produce(cg, input);
  // parent->produce(cg);
  input->produce(cg);
}

InputType InputOperator::getInputType() { return type; }

std::string InputOperator::getInputTypeAsString() {
  if (type == BinaryFile)
    return "BinaryFile";
  else
    return "";
}

std::string InputOperator::getPath() { return path; }

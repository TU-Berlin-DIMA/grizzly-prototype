#include <sstream>
#include <string>

#include "operator/KeyOperator.h"

KeyOperator::KeyOperator(Field &field, Operator *input) : field(field), input(input) {
  leftChild = NULL;
  rightChild = NULL;
  input->parent = this;
  name = "Key";
}

std::string KeyOperator::to_string() { return "Key by " + field.name; }

KeyOperator::~KeyOperator() {}

void KeyOperator::consume(CodeGenerator &cg) {
  if (parent != nullptr) {
    parent->consume(cg);
  }
}

void KeyOperator::produce(CodeGenerator &cg) {
  // add KeyBy-Field to the query context of the pipeline
  pipeline = cg.currentPipeline();
  cg.ctx(pipeline).keyBy = &field;
  cg.ctx(pipeline).hasKeyBy = true;

  input->produce(cg);
}

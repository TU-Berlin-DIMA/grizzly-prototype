#include <sstream>
#include <string>

#include "operator/MapOperator.h"

MapOperator::MapOperator(Mapper &mapper, Operator *input) : mapper(mapper), input(input) {

  leftChild = NULL;
  rightChild = NULL;
  input->parent = this;
  name = "Map";
}

std::string MapOperator::to_string() { return "Map"; }

MapOperator::~MapOperator() { delete input; }

void MapOperator::consume(CodeGenerator &cg) {
  // delegate to specific mapper function
  mapper.consume(cg, parent);
}

void MapOperator::produce(CodeGenerator &cg) {
  // delegate to specific mapper function
  mapper.produce(cg, input);
}

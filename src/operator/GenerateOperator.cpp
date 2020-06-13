#include <sstream>
#include <string>

#include "operator/GenerateOperator.h"

GenerateOperator::GenerateOperator(Operator *input) : input(input) {
  leftChild = NULL;
  rightChild = NULL;
  input->parent = this;
  name = "Generate";
}

std::string GenerateOperator::to_string() { return "Generate"; }

GenerateOperator::~GenerateOperator() {}

void GenerateOperator::consume(CodeGenerator &cg) {
  if (parent != nullptr) {
    parent->consume(cg);
  }
}

void GenerateOperator::produce(CodeGenerator &cg) {
  // generate Yahoo Benchmark Data
  //	cg.main.addStatement("runtime::generateYahoo(1000);");
  input->produce(cg);
}

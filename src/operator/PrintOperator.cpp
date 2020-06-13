#include <sstream>
#include <string>

#include "operator/PrintOperator.h"

PrintOperator::PrintOperator(Operator *input) : input(input) {
  leftChild = NULL;
  rightChild = NULL;
  name = "Print";
  input->parent = this;
}

std::string PrintOperator::to_string() { return "Print"; }

PrintOperator::~PrintOperator() { delete input; }

void PrintOperator::consume(CodeGenerator &cg) {
  std::stringstream statements;
  statements << "std::cout ";

  if (cg.ctx(pipeline + 1).hasGroupBy) {
    statements << "<< key << \":\" ";
  }

  // print each field of the schema
  for (auto &field : cg.ctx(pipeline + 1).schema.fields) {
    statements << " << record." << field.name << " << \"|\" ";
  }

  statements << "<< std::endl;";
  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_PRINT, statements.str()));
}

void PrintOperator::produce(CodeGenerator &cg) {
  pipeline = cg.currentPipeline();
  input->produce(cg);
}

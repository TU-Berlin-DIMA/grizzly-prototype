#include <sstream>
#include <string>

#include "operator/ReadOperator.h"

ReadOperator::ReadOperator(Schema &schema) : schema(schema) {
  leftChild = NULL;
  rightChild = NULL;
  name = "Read";
}

std::string ReadOperator::to_string() { return "Read Schema"; }

ReadOperator::~ReadOperator() {}

void ReadOperator::consume(CodeGenerator &cg) {
  // Leaf operator; no consume function
}

void ReadOperator::produce(CodeGenerator &cg) {
  pipeline = cg.currentPipeline();
  std::string recordType = "record0";

  // make records available for next operators
  cg.pipeline(pipeline).addParameter(recordType + "* records").addParameter("size_t size");

  // for-loop
  std::stringstream statements;
  statements << "for(size_t i = 0; i<size; ++i) {" << std::endl;
  statements << recordType << " &record = records[i];" << std::endl;
  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_READ, statements.str()));

  if (parent != nullptr) {
    parent->consume(cg);
  }

  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_CLOSE, std::string("}\n")));
}

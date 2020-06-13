#include <sstream>
#include <string>

#include "operator/WriteOperator.h"

WriteOperator::WriteOperator(std::string fileName, Operator *input) : fileName(fileName), input(input) {
  leftChild = NULL;
  rightChild = NULL;
  name = "Write";
  input->parent = this;
}

std::string WriteOperator::to_string() { return "Write File " + fileName; }

WriteOperator::~WriteOperator() { delete input; }

void WriteOperator::consume(CodeGenerator &cg) {
  cg.file.include("fstream");

  auto init = std::string("");
  auto code = std::string("");
  auto final = std::string("");

  init += "std::ofstream file;\n";
  init += "file.open(\"" + fileName + "\", std::ios::out | std::ios::app);\n";

  code += std::string("file");

  /* Print the Groub Key, if Operator before was Grouped Aggregation. */
  if (cg.ctx(pipeline + 1).hasGroupBy) {
    code += " << key";
  }

  /* Print each field of Schema. */
  for (auto &field : cg.ctx(pipeline + 1).schema.fields) {
    code += " << \";\" << record." + field.name;
  }
  code += "<< \"\\n\";\n";

  final += "file.close();\n";

  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_WRITE, init, code, final));
}

void WriteOperator::produce(CodeGenerator &cg) {
  pipeline = cg.currentPipeline();
  input->produce(cg);
}

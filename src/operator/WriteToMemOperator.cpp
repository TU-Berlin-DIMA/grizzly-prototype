#include <sstream>
#include <string>

#include "operator/WriteToMemOperator.h"

WriteToMemOperator::WriteToMemOperator(Operator *input) : input(input) {
  leftChild = NULL;
  rightChild = NULL;
  name = "WriteToMem";
  input->parent = this;
}

std::string WriteToMemOperator::to_string() { return "WriteToMem"; }

WriteToMemOperator::~WriteToMemOperator() { delete input; }

void WriteToMemOperator::consume(CodeGenerator &cg) {

  auto &record = cg.pipeline(pipeline).parameters[0];
  std::stringstream statements;
  cg.ctx(cg.currentPipeline()).outputSchema.print();

  for (auto f : cg.ctx(cg.currentPipeline()).outputSchema.fields) {
    statements << "buffer[thread_id]." << f.name << "=record." << f.name << ";" << std::endl;
  }

  cg.generateStruct(cg.ctx(cg.currentPipeline()).outputSchema, "output", cg.currentPipeline(), false);

  std::stringstream intBufferStatement;
  intBufferStatement << "auto buffer = (output" << cg.currentPipeline() << "*) malloc (sizeof(output"
                     << cg.currentPipeline() << ")*1000);";
  intBufferStatement << "int b_i = 0;";
  cg.file.addStatement(intBufferStatement.str());

  statements << std::endl;
  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_PRINT, statements.str()));
}

void WriteToMemOperator::produce(CodeGenerator &cg) {
  pipeline = cg.currentPipeline();
  input->produce(cg);
}

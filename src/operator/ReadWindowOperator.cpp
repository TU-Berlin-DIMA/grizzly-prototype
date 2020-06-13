#include <sstream>
#include <string>

#include "operator/ReadWindowOperator.h"

ReadWindowOperator::ReadWindowOperator(Schema &schema, Operator *input) : schema(schema), input(input) {
  leftChild = NULL;
  rightChild = NULL;
  name = "Read Window";
  input->parent = this;
}

std::string ReadWindowOperator::to_string() { return "Read Window"; }

ReadWindowOperator::~ReadWindowOperator() {}

void ReadWindowOperator::consume(CodeGenerator &cg) {
  std::string resultType = "record" + std::to_string(pipeline + 1);

  // if non-grouping query, only read from result record
  if (!cg.ctx(pipeline + 1).hasGroupBy || cg.ctx(pipeline + 1).hasKeyBy) {
    cg.pipeline(pipeline).addParameter(resultType + " record");
    if (parent != nullptr) {
      parent->consume(cg);
    }
  }

  // in grouping query, read from map
  std::stringstream statements;
  if (cg.ctx(pipeline + 1).hasGroupBy) {
    if (cg.ctx(pipeline + 1).maxKeyValue != -1) {

      cg.pipeline(pipeline).addParameter("size_t currentWindow");

      statements << "for (size_t i = 0; i < " << cg.ctx(pipeline + 1).maxKeyValue << "; i++)  {" << std::endl;
      statements << " auto key = i;" << std::endl;
      if (cg.ctx(pipeline + 1).stateStrategy == QueryContext::SHARED) {
        statements << " auto record = state" << (pipeline + 1) << "[currentWindow][i];" << std::endl;
      } else {
        statements << "// merge independent window states\n";
        statements << " auto record = state" << (pipeline + 1) << "[currentWindow][i];" << std::endl;
        statements << " for(int mergeThreadID = 1; mergeThreadID < " << cg.config.getParallelism()
                   << "; mergeThreadID++){"
                      "auto temp = state"
                   << (pipeline + 1) << "[currentWindow + (mergeThreadID * window_buffers" << (pipeline + 1) << ")][i];";
        for (auto field : cg.ctx(pipeline+1).schema.fields) {
          statements << "record." << field.name << " = "
                     << "temp." << field.name << ";\n";
        }
        statements << "}";
      }
    } else {
      std::string keyType = cg.ctx(pipeline + 1).groupBy->dataType.keyType();
      cg.pipeline(pipeline).addParameter("tbb::concurrent_unordered_map<" + keyType + ", " + resultType + "> records");
      statements << "for (auto const &it : records) {" << std::endl;
      statements << keyType << " key = it.first;" << std::endl;
      statements << resultType << " record = it.second;" << std::endl;
    }

    cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_READ, statements.str()));

    if (parent != nullptr) {
      parent->consume(cg);
    }
    cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_CLOSE, std::string("}\n")));
  }
}

void ReadWindowOperator::produce(CodeGenerator &cg) {
  pipeline = cg.currentPipeline();
  input->produce(cg);
}

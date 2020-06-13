#include <iostream>
#include <sstream>
#include <string>

#include "api/Predicate.h"

void Predicate::produce(CodeGenerator &cg, Operator *input) {
  // get current pipeline id and save it for the consume-function
  pipeline = cg.currentPipeline();

  // get the field to the field id
  if (pipeline == 0)
    field = &cg.ctx(pipeline).schema.get(fieldId);
  else
    field = &cg.ctx(pipeline - 1).schema.get(fieldId);
  // call produce for all and/or predicates
  for (Predicate *a : ands) {
    a->produce(cg, NULL);
  }
  for (Predicate *o : ors) {
    o->produce(cg, NULL);
  }

  if (input)
    input->produce(cg);
}

void Predicate::consume(CodeGenerator &cg, Operator *parent) {
  cg.file.include("string.h");

  std::stringstream front_statements;

  if (cg.config.filterOpt() && cg.compileMode == CM_INSTRUMENT) {
    auto code = generatProfilingCode(cg);
    cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_FILTER, code));
  }

  if (cg.config.filterOpt() && cg.compileMode == CM_OPTIMIZE) {
    front_statements << "if(" << generateAllOptimized(cg) << ") {" << std::endl;
  } else {
    front_statements << "if(" << generateAll(cg) << ") {" << std::endl;
  }
  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_FILTER, front_statements.str()));

  if (parent != nullptr) {
    parent->consume(cg);
  }

  std::stringstream back_statements;
  back_statements << "}" << std::endl;
  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_CLOSE, back_statements.str()));
}

std::string Equal::generate(CodeGenerator &c) {
  std::string fieldId = "record." + field->name;

  switch (field->dataType.t_) {
  case DataType::String:
  case DataType::Char: {
    if (c.compileMode == CM_OPTIMIZE) {
      c.file.addStatement("static const char viewChar[] = " + value + ";");
      return "memcmp(" + fieldId + ", viewChar, 4) == 0";
    } else {
      return "strcmp(" + fieldId + ", " + value + ") == 0";
    }
  }
  case DataType::Boolean:
  case DataType::Int:
  case DataType::Long:
  case DataType::Double:
    return fieldId + " == " + value;
  default:
    throw std::invalid_argument("unsupported data type");
  }
}

std::string Equal::to_string() { return fieldId + " = " + value; }

std::string NotEqual::generate(CodeGenerator &c) {
  std::string fieldId = "record." + field->name;

  switch (field->dataType.t_) {
  case DataType::String:
  case DataType::Char:
    return "strcmp(" + fieldId + ", " + value + ") != 0";
  case DataType::Boolean:
  case DataType::Int:
  case DataType::Long:
  case DataType::Double:
    return fieldId + " != " + value;
  default:
    throw std::invalid_argument("unsupported data type");
  }
}

std::string NotEqual::to_string() { return fieldId + " != " + value; }

std::string Greater::generate(CodeGenerator &c) {
  std::string fieldId = "record." + field->name;

  switch (field->dataType.t_) {
  case DataType::String:
  case DataType::Char:
    return "strcmp(" + fieldId + ", " + value + ") > 0";
  case DataType::Boolean:
  case DataType::Int:
  case DataType::Long:
  case DataType::Double:
    return fieldId + " > " + value;
  default:
    throw std::invalid_argument("unsupported data type");
  }
}

std::string Greater::to_string() { return fieldId + " > " + value; }

std::string GreaterEqual::generate(CodeGenerator &c) {
  std::string fieldId = "record." + field->name;

  switch (field->dataType.t_) {
  case DataType::String:
  case DataType::Char:
    return "strcmp(" + fieldId + ", " + value + ") >= 0";
  case DataType::Boolean:
  case DataType::Int:
  case DataType::Long:
  case DataType::Double:
    return fieldId + " >= " + value;
  default:
    throw std::invalid_argument("unsupported data type");
  }
}

std::string GreaterEqual::to_string() { return fieldId + " >= " + value; }

std::string Less::generate(CodeGenerator &c) {
  std::string fieldId = "record." + field->name;

  switch (field->dataType.t_) {
  case DataType::String:
  case DataType::Char:
    return "strcmp(" + fieldId + ", " + value + ") < 0";
  case DataType::Boolean:
  case DataType::Int:
  case DataType::Long:
  case DataType::Double:
    return fieldId + " < " + value;
  default:
    throw std::invalid_argument("unsupported data type");
  }
}

std::string Less::to_string() { return fieldId + " < " + value; }

std::string LessEqual::generate(CodeGenerator &c) {
  std::string fieldId = "record." + field->name;

  switch (field->dataType.t_) {
  case DataType::String:
  case DataType::Char:
    return "strcmp(" + fieldId + ", " + value + ") <= 0";
  case DataType::Boolean:
  case DataType::Int:
  case DataType::Long:
  case DataType::Double:
    return fieldId + " <= " + value;
  default:
    throw std::invalid_argument("unsupported data type");
  }
}

std::string LessEqual::to_string() { return fieldId + " < " + value; }

std::string Like::generate(CodeGenerator &c) {
  std::string fieldId = "record." + field->name;

  switch (field->dataType.t_) {
  case DataType::String:
  case DataType::Char:
    return "strstr(" + fieldId + ", " + value + ") != NULL";
  default:
    throw std::invalid_argument("unsupported data type");
  }
}

std::string Like::to_string() { return fieldId + " like " + value; }

#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include "code_generation/CMethod.h"

/*
 * CMethod Definition
 */
CMethod::CMethod(CMethod::Builder &builder) {
  std::stringstream method;

  // build method signature
  method << std::endl << builder.returnType << " " << builder.name << "(";
  for (size_t i = 0; i < builder.parameters.size(); ++i) {
    if (i != 0)
      method << ",";
    method << builder.parameters[i];
  }
  method << ") {" << std::endl;

  // build statements
  std::stringstream statements;
  std::stringstream init_statements;
  std::stringstream final_statements;

  for (auto const &instruction : builder.instructions) {
    statements << instruction.statements;
    init_statements << instruction.init_statements;
    final_statements << instruction.final_statements;
  }
  method << init_statements.str() << statements.str() << final_statements.str() << "}" << std::endl;
  output = method.str();
}

CMethod::Builder CMethod::builder() { return CMethod::Builder(); }

/*
 * CMethod Instruction Definition
 */
CMethod::Instruction::Instruction(INSTRUCTION type, std::string init, std::string statements, std::string final)
    : type(type), init_statements(init), statements(statements), final_statements(final) {}
CMethod::Instruction::Instruction(INSTRUCTION type, std::string statements, std::string final)
    : type(type), init_statements(std::string()), statements(statements), final_statements(final){};
CMethod::Instruction::Instruction(INSTRUCTION type, std::string statements)
    : type(type), init_statements(std::string()), statements(statements), final_statements(std::string()){};

const std::string CMethod::Instruction::to_string() const {
  switch (type) {
  case INSTRUCTION_FILTER:
    return "FILTER[" + statements.substr(3, statements.size() - 6) + "]\n";
  case INSTRUCTION_GROUPBY:
    return "GROUPBY";
  case INSTRUCTION_ORDERBY:
    return "ORDERBY";
  case INSTRUCTION_AGGREGATE:
    return "AGGREGATE";
  case INSTRUCTION_JOIN_BUILD:
    return "JOIN_BUILD";
  case INSTRUCTION_JOIN_PROBE:
    return "JOIN_PROBE";
  case INSTRUCTION_READ:
    return "READ";
  case INSTRUCTION_WRITE:
    return "WRITE";
  case INSTRUCTION_PRINT:
    return "PRINT";
  case INSTRUCTION_TRIGGER:
    return "TRIGGER";
  case INSTRUCTION_ASSIGNER:
    return "ASSIGNER";
  case INSTRUCTION_CLOSE:
    return "CLOSE";
  default:
    return "UNKNOWN_INSTRUCTION";
  }
}

/*
 * CMethod Builder Definition
 */
CMethod::Builder::Builder() {}

CMethod::Builder &CMethod::Builder::withName(const std::string &name_) {
  name = name_;
  return *this;
}

CMethod::Builder &CMethod::Builder::returns(const std::string &returnType_) {
  returnType = returnType_;
  return *this;
}

CMethod::Builder &CMethod::Builder::addParameter(const std::string &parameter) {
  parameters.push_back(parameter);
  return *this;
}

CMethod::Builder &CMethod::Builder::prependInstruction(const Instruction &instruction) {
  instructions.insert(instructions.begin(), instruction);
  return *this;
}

CMethod::Builder &CMethod::Builder::addInstruction(const Instruction &instruction) {
  instructions.push_back(instruction);
  return *this;
}

CMethod CMethod::Builder::build() { return CMethod(*this); }

/*
 * CMethod PlanEnumerator Definition
 */
CMethod::PipelineEnumerator::PipelineEnumerator(std::vector<Instruction> pipeline_instructions) {

  /* Count permutable instructions and copy them into vector. */
  unsigned int i = 0;
  for (auto const &instruction : pipeline_instructions) {
    /* Currently only filter instructions are supported. */
    if (instruction.type == INSTRUCTION_FILTER) {
      permutable_positions.push_back(i);
      current_permutation.push_back(instruction);
    }
    i++;
  }

  /* Managing starting point of permutation enumeration. */
  number_of_instructions = current_permutation.size();
  number_of_permutations = factorial(current_permutation.size());
  number_of_current_permutation = 0;
}

CMethod::PipelineEnumerator::PipelineEnumerator(CMethod::Builder &builder)
    : CMethod::PipelineEnumerator(builder.instructions) {}

void CMethod::PipelineEnumerator::getPermutation(CMethod::Builder &builder, unsigned int number_of_permutation) {

  assert(number_of_permutation < number_of_permutations);

  /* Permute the order of statements till it meets the given number. */
  while (number_of_current_permutation != number_of_permutation) {
    getNext();
  }

  /* Push permuted instructions into corresponding positions of instruction vector. */
  for (unsigned int i = 0; i != permutable_positions.size(); ++i) {
    const size_t position = permutable_positions[i];
    builder.instructions[position] = current_permutation[i];
  }
}

void CMethod::PipelineEnumerator::printPermutations() {
  std::cout << "Pipeline consists of " << number_of_instructions << " permutable instructions." << std::endl;
  std::cout << "There are " << number_of_permutations << " permuations of this pipeline." << std::endl;

  /* Print permutation. */
  for (unsigned int i = 0; i != number_of_permutations; i++) {
    std::cout << "Permutation " << i << ":" << std::endl;
    for (auto const &instruction : current_permutation) {
      std::cout << "\t" << instruction.to_string();
    }
    getNext();
  }
  std::cout << std::endl;
}

void CMethod::PipelineEnumerator::getNext() {
  std::next_permutation(current_permutation.begin(), current_permutation.end());
  number_of_current_permutation = (number_of_current_permutation + 1) % number_of_permutations;
}

unsigned int CMethod::PipelineEnumerator::factorial(unsigned int n) {
  return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

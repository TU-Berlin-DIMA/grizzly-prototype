#ifndef CODE_GENERATION_C_METHOD_H
#define CODE_GENERATION_C_METHOD_H

#include <string>
#include <tuple>
#include <vector>

enum INSTRUCTION {
  INSTRUCTION_FILTER,
  INSTRUCTION_GROUPBY,
  INSTRUCTION_ORDERBY,
  INSTRUCTION_AGGREGATE,
  INSTRUCTION_JOIN_BUILD,
  INSTRUCTION_JOIN_PROBE,
  INSTRUCTION_READ,
  INSTRUCTION_WRITE,
  INSTRUCTION_PRINT,
  INSTRUCTION_TRIGGER,
  INSTRUCTION_ASSIGNER,
  INSTRUCTION_CLOSE, // Closing Brackets etc.
  INSTRUCTION_SYSTEM
};

class CMethod {

public:
  class Instruction {
  public:
    Instruction(INSTRUCTION type, std::string init, std::string statements, std::string final);
    Instruction(INSTRUCTION type, std::string statements, std::string final);
    Instruction(INSTRUCTION type, std::string statements);

    INSTRUCTION type;
    std::string init_statements;
    std::string statements;
    std::string final_statements;

    const std::string to_string() const;

    inline bool operator<(const Instruction &other) const { return statements < other.statements; }
  };

  class Builder {
  public:
    std::string name;
    std::string returnType;
    std::vector<std::string> parameters;
    std::vector<Instruction> instructions;

    Builder();
    Builder &withName(const std::string &name_);
    Builder &returns(const std::string &returnType_);
    Builder &addParameter(const std::string &parameter);
    Builder &prependInstruction(const Instruction &instruction);
    Builder &addInstruction(const Instruction &instruction);
    CMethod build();
  };

  class PipelineEnumerator {
  public:
    PipelineEnumerator(std::vector<Instruction> instructions);
    PipelineEnumerator(Builder &builder);
    void printPermutations();
    void getPermutation(CMethod::Builder &builder, unsigned int number_of_permutation);

  private:
    unsigned int number_of_instructions;
    unsigned int number_of_permutations;
    std::vector<unsigned int> permutable_positions;
    size_t number_of_current_permutation;
    std::vector<Instruction> current_permutation;

    void getNext();

    static unsigned int factorial(unsigned int n);
  };

  std::string output;
  static CMethod::Builder builder();

private:
  CMethod(CMethod::Builder &builder);
};

#endif // CODE_GENERATION_C_METHOD_H

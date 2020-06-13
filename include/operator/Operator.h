#ifndef OPERATOR_OPERATOR_H
#define OPERATOR_OPERATOR_H

#include <string>

#include "api/Field.h"
#include "api/Schema.h"
#include "code_generation/CodeGenerator.h"
#include "runtime/input_types.h"

class Operator {
public:
  virtual ~Operator() {}
  virtual void produce(CodeGenerator &cg) = 0;
  virtual void consume(CodeGenerator &cg) = 0;
  Operator *parent;
  Operator *leftChild;
  Operator *rightChild;
  std::string name;
  size_t pipeline;
  size_t cost;

  virtual std::string to_string() { return "Operator"; }
};

#endif // OPERATOR_OPERATOR_H

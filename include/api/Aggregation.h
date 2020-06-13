#ifndef API_AGGREGATION_H
#define API_AGGREGATION_H

#include "api/Field.h"
#include "code_generation/CodeGenerator.h"
#include "operator/MapOperator.h"
#include "operator/Operator.h"

class Aggregation {
public:
  Aggregation() {}
  Aggregation(std::string fieldId) : fieldId(fieldId) {}

  virtual void produce(CodeGenerator &cg, Operator *input) = 0;
  virtual void consume(CodeGenerator &cg, Operator *input) = 0;

  virtual std::string to_string() { return "Aggregation"; }
  virtual bool hasFinalAggregation() { return false; }

  virtual void consumeFinalAggregation(CodeGenerator &generator, Operator *pOperator);

  virtual void produceFinalAggregation(CodeGenerator &generator, Operator *pOperator);

protected:
  size_t pipeline;
  std::string fieldId;
  void consume_(CodeGenerator &cg, Operator *input);
  void produce_(CodeGenerator &cg, Operator *input, Schema &schema);
  void createState(CodeGenerator &cg, Operator *input, Schema &schema);
  void migrateFrom(CodeGenerator &cg, Operator *input, Schema &schema);
  void migrateTo(CodeGenerator &cg, Operator *input, Schema &schema);
  void addStatePtr(CodeGenerator &cg, Operator *input, Schema &schema);
};

class Sum : public Aggregation {
public:
  Sum(std::string fieldId) : Aggregation(fieldId) {}

  std::string to_string() { return "Sum(" + fieldId + ")"; };
  void produce(CodeGenerator &cg, Operator *input);
  void consume(CodeGenerator &cg, Operator *input);
};

class Count : public Aggregation {
public:
  Count() : Aggregation() {}

  std::string to_string() { return "Count(" + fieldId + ")"; };
  void produce(CodeGenerator &cg, Operator *input);
  void consume(CodeGenerator &cg, Operator *input);
};

class Min : public Aggregation {
public:
  Min(std::string fieldId) : Aggregation(fieldId) {}

  std::string to_string() { return "Min(" + fieldId + ")"; };
  void produce(CodeGenerator &cg, Operator *input);
  void consume(CodeGenerator &cg, Operator *input);
};

class Max : public Aggregation {
public:
  Max(std::string fieldId) : Aggregation(fieldId) {}

  std::string to_string() { return "Max(" + fieldId + ")"; };
  void produce(CodeGenerator &cg, Operator *input);
  void consume(CodeGenerator &cg, Operator *input);
};

class Avg : public Aggregation {
public:
  Avg(std::string fieldId) : Aggregation(fieldId) {}

  bool hasFinalAggregation() override;

  std::string to_string() { return "Avg(" + fieldId + ")"; };
  void produce(CodeGenerator &cg, Operator *input);
  void consume(CodeGenerator &cg, Operator *input);
  void consumeFinalAggregation(CodeGenerator &cg, Operator *pOperator);
};

#endif // API_AGGREGATION_H

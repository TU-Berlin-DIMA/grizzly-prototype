#include <sstream>
#include <string>

#include "operator/GroupByOperator.h"

GroupByOperator::GroupByOperator(Field &field, Operator *input) : field(field), input(input), maxValue(-1) {
  leftChild = NULL;
  rightChild = NULL;
  input->parent = this;
  name = "Group";
}

GroupByOperator::GroupByOperator(Field &field, Operator *input, int maxValue)
    : field(field), input(input), maxValue(maxValue) {
  leftChild = NULL;
  rightChild = NULL;
  input->parent = this;
  name = "Group";
}

std::string GroupByOperator::to_string() { return "Group by " + field.name; }

GroupByOperator::~GroupByOperator() {}

void GroupByOperator::consume(CodeGenerator &cg) {
  if (parent != nullptr) {
    parent->consume(cg);
  }
}

void GroupByOperator::produce(CodeGenerator &cg) {
  // add GroupBy-Field to the query context of the pipeline
  pipeline = cg.currentPipeline();
  cg.ctx(pipeline).groupBy = &field;
  cg.ctx(pipeline).hasGroupBy = true;
  if (cg.compileMode == CM_OPTIMIZE) {
    auto profiledMax = cg.profilingDataManager->getMaxHandler("agg_max")->getValue();
    auto profiledMin = cg.profilingDataManager->getMaxHandler("agg_min")->getValue();
    cg.ctx(pipeline).maxKeyValue = profiledMax;
  }

  // cg.ctx(pipeline).maxKeyValue = maxValue;

  input->produce(cg);
}

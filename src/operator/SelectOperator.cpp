#include <sstream>
#include <string>

#include "operator/SelectOperator.h"

SelectOperator::SelectOperator(Operator *input, std::vector<std::string> fields) : fields(fields), input(input) {
  leftChild = NULL;
  rightChild = NULL;
  input->parent = this;
  name = "Select";
}

std::string SelectOperator::to_string() { return "Select"; }

SelectOperator::~SelectOperator() { delete input; }

void SelectOperator::consume(CodeGenerator &cg) {
  if (parent != nullptr) {
    parent->consume(cg);
  }
}

void SelectOperator::produce(CodeGenerator &cg) {
  // add KeyBy-Field to the query context of the pipeline
  pipeline = cg.currentPipeline();
  Schema schema = Schema::create();
  for (Field s : cg.ctx(0).schema.fields) {
    std::string name = s.name;
    for (std::string otherField : fields) {
      if (name.compare(otherField) == 0) {
        schema.fields.push_back(s);
      }
    }
  }

  cg.ctx(cg.currentPipeline()).outputSchema = schema;
  QueryContext qx = QueryContext(schema);
  cg.generateStruct(schema, "output", cg.currentPipeline(), false);
  input->produce(cg);
}

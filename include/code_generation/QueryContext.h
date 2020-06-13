#ifndef CODE_GENERATOR_QUERYCONTEXT_H
#define CODE_GENERATOR_QUERYCONTEXT_H

#include <vector>

#include "api/Schema.h"

class QueryContext {
public:
  QueryContext(Schema schema) : schema(schema), outputSchema(schema) {}

  enum StateStrategy { INDEPENDENT, SHARED };

  Schema schema;
  Schema outputSchema;
  std::vector<Field> output;
  Field *keyBy;
  Field *groupBy;
  size_t numWindows = 0;
  bool hasKeyBy = false;
  bool hasGroupBy = false;
  bool isAggregation = false;
  int maxKeyValue = -1;
  StateStrategy stateStrategy = SHARED;
};

#endif // CODE_GENERATOR_QUERYCONTEXT_H

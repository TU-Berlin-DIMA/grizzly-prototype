#ifndef API_SCHEMA_H
#define API_SCHEMA_H

#include <string>
#include <vector>

#include "api/Field.h"

class Schema {
public:
  static Schema create();
  Schema &addFixSizeField(std::string name, DataType dataType, SourceType srcType);
  Schema &addVarSizeField(std::string name, DataType dataType, size_t dataSize, SourceType srcType);
  Field &get(std::string name);
  Schema &print();
  std::vector<Field> fields;
  size_t getInputSize();

private:
  Schema();
};

#endif // API_SCHEMA_H

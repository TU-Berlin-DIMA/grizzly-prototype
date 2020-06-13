#include <iostream>
#include <stdexcept>

#include "api/Schema.h"

Schema::Schema() {}

Schema Schema::create() { return Schema(); }

size_t Schema::getInputSize() {
  return 78;
  // TODO:make it dynamic
}
Schema &Schema::addFixSizeField(std::string name, DataType dataType, SourceType srcType) {
  fields.push_back(Field(name, dataType, dataType.defaultSize(), srcType));
  return *this;
}

Schema &Schema::addVarSizeField(std::string name, DataType dataType, size_t dataSize, SourceType srcType) {
  fields.push_back(Field(name, dataType, dataSize, srcType));
  return *this;
}

Field &Schema::get(std::string pName) {
  for (auto &f : fields) {
    if (f.name == pName)
      return f;
  }

  throw std::invalid_argument("field " + pName + " does not exist");
}

Schema &Schema::print() {
  // todo
  return *this;
}

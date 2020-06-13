#include "api/Field.h"

Field::Field(std::string name, DataType type, std::size_t size, SourceType srcType)
    : name(name), dataType(type), size(size), srcType(srcType) {}

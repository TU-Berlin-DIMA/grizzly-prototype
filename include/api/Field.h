#ifndef API_FIELD_H
#define API_FIELD_H

#include <stdexcept>
#include <string>

enum SourceType { Stream, Table };

struct DataType {
  enum Type { Boolean, Char, String, Int, Long, Double };
  Type t_;

  DataType(Type t) : t_(t) {}

  operator Type() const { return t_; }

  size_t defaultSize() {
    switch (t_) {
    case DataType::String:
      return 255;
    default:
      return 1;
    }
  }

  const std::string cType() const {
    switch (t_) {
    case DataType::Boolean:
      return "bool";
    case DataType::Char:
      return "char";
    case DataType::String:
      return "char";
    case DataType::Int:
      return "int";
    case DataType::Long:
      return "long";
    case DataType::Double:
      return "double";
    default:
      throw std::invalid_argument("data type not supported");
    }
  }

  const std::string keyType() const {
    switch (t_) {
    case DataType::Boolean:
      return "bool";
    case DataType::Char:
    case DataType::String:
      return "std::string";
    case DataType::Int:
      return "int";
    case DataType::Long:
      return "long";
    case DataType::Double:
      return "double";
    default:
      throw std::invalid_argument("data type not supported");
    }
  }

private:
  template <typename T> operator T() const;
};

class Field {
public:
  Field(std::string name, DataType dataType, std::size_t dataSize, SourceType srcType);
  std::string name;
  DataType dataType;
  std::size_t size;
  SourceType srcType;
};

#endif // API_FIELD_H

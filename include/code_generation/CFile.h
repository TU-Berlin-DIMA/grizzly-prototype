#ifndef CODE_GENERATION_C_FILE_H
#define CODE_GENERATION_C_FILE_H

#include <string>
#include <vector>

#include "code_generation/CCode.h"
#include "code_generation/CMethod.h"

class CFile {
public:
  class Builder {
  public:
    std::string name;
    std::vector<std::string> includes;
    std::vector<CCode> codes;
    std::vector<CMethod> methods;

    Builder();
    Builder &withName(const std::string &name_);
    Builder &include(const std::string &include);
    Builder &addStatement(const std::string &statement);
    Builder &addCode(CCode &code);
    Builder &addMethod(CMethod &method);
    CFile build();
  };

  std::string name;
  std::string output;
  static CFile::Builder builder();

private:
  CFile(CFile::Builder &builder);
};

#endif // CODE_GENERATION_C_FILE_H

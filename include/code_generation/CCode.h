#ifndef CODE_GENERATION_C_CODE_H
#define CODE_GENERATION_C_CODE_H

#include <string>
#include <vector>

class CCode {

public:
  class Builder {
  public:
    std::vector<std::string> statements;

    Builder();
    Builder &addStatement(const std::string &statement);
    Builder &beginControlFlow(const std::string &statement);
    Builder &endControlFlow();
    CCode build();
  };

  std::string output;
  static CCode::Builder builder();

private:
  CCode(CCode::Builder &builder);
};

#endif // CODE_GENERATION_C_CODE_H

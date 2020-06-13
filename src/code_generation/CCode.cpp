#include <iostream>
#include <sstream>
#include <string>

#include "code_generation/CCode.h"

/*
 * CCode Definition
 */
CCode::CCode(CCode::Builder &builder) {
  std::stringstream ss;

  // build statements
  for (std::string statement : builder.statements) {
    ss << statement << std::endl;
  }

  output = ss.str();
}

CCode::Builder CCode::builder() { return CCode::Builder(); }

/*
 * CCode Builder Definition
 */
CCode::Builder::Builder() {}

CCode::Builder &CCode::Builder::addStatement(const std::string &statement) {
  statements.push_back(statement);
  return *this;
}

CCode::Builder &CCode::Builder::beginControlFlow(const std::string &statement) {
  statements.push_back(statement + "{");
  return *this;
}

CCode::Builder &CCode::Builder::endControlFlow() {
  statements.push_back("}");
  return *this;
}

CCode CCode::Builder::build() { return CCode(*this); }

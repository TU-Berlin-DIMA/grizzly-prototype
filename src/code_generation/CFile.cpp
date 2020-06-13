#include <iostream>
#include <sstream>
#include <string>

#include "code_generation/CFile.h"

/*
 * CFile Definition
 */
CFile::CFile(CFile::Builder &builder) {

  name = builder.name;

  std::stringstream ss;

  // pragma once for header files
  if (name.substr(name.length() - 2) == ".h")
    ss << "#pragma once" << std::endl;

  // includes
  for (std::string include : builder.includes) {
    ss << "#include \"" + include + "\"" << std::endl;
  }

  // code
  for (CCode code : builder.codes) {
    ss << code.output;
  }

  // generate method code
  for (CMethod method : builder.methods) {
    ss << method.output;
  }

  output = ss.str();
}

CFile::Builder CFile::builder() { return CFile::Builder(); }

/*
 * CFile Builder Definition
 */
CFile::Builder::Builder() { includes.push_back("iostream"); }

CFile::Builder &CFile::Builder::withName(const std::string &name_) {
  name = name_;
  return *this;
}

CFile::Builder &CFile::Builder::include(const std::string &include) {
  includes.push_back(include);
  return *this;
}

CFile::Builder &CFile::Builder::addStatement(const std::string &statement) {
  CCode code = CCode::builder().addStatement(statement).build();
  codes.push_back(code);
  return *this;
}

CFile::Builder &CFile::Builder::addCode(CCode &code) {
  codes.push_back(code);
  return *this;
}

CFile::Builder &CFile::Builder::addMethod(CMethod &method) {
  methods.push_back(method);
  return *this;
}

CFile CFile::Builder::build() { return CFile(*this); }

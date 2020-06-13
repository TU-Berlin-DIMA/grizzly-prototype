#ifndef CODE_JITGENERATION_CODE_GENERATOR_H
#define CODE_JITGENERATION_CODE_GENERATOR_H

#include "api/Config.h"
#include "api/Field.h"
#include "code_generation/CCode.h"
#include "code_generation/CFile.h"
#include "code_generation/CMethod.h"
#include "code_generation/CodeGenerator.h"
#include "code_generation/QueryContext.h"
#include "runtime/input_types.h"

class JITCodeGenerator : public CodeGenerator {
public:
  JITCodeGenerator(Config &config, Schema &schema, ProfilingDataManager *profilingDataManager, CompileMode mode);

  CFile generate(std::string type, std::string path);

  void generateStructFile(std::string path);
};

#endif // CODE_JITGENERATION_CODE_GENERATOR_H

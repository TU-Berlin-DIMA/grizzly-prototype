#ifndef CODE_GENERATION_CODE_GENERATOR_H
#define CODE_GENERATION_CODE_GENERATOR_H

#include "api/Config.h"
#include "api/Field.h"
#include "code_generation/CCode.h"
#include "code_generation/CFile.h"
#include "code_generation/CMethod.h"
#include "code_generation/QueryContext.h"
#include "jit/runtime/Profiling.h"

enum CompileMode { CM_DEFAULT, CM_INSTRUMENT, CM_OPTIMIZE };

class CodeGenerator {
public:
  CodeGenerator(Config &config, Schema &schema, CompileMode mode);

  CFile generate(std::string type, std::string path);

  void compileCode();

  void run();

  CMethod::Builder &pipeline(size_t id);

  QueryContext &ctx(size_t id);

  void addQueryContext(QueryContext ctx);

  void startPipeline();

  size_t currentPipeline();

  size_t longestPipeline();

  CCode generateStruct(Schema &schema, std::string name, size_t id, bool useAtomic);

  Config &config;
  CMethod::Builder main;
  CMethod::Builder open;
  CMethod::Builder migrateFrom;
  CMethod::Builder migrateTo;
  CMethod::Builder init;
  CMethod::Builder execute;
  CMethod::Builder getState;
  CFile::Builder file;
  ProfilingDataManager *profilingDataManager;
  CompileMode compileMode;

  void writeToFile(CFile &file);

protected:
  std::vector<CMethod::Builder> pipelines;
  std::vector<QueryContext> queryContexts;
  std::vector<CCode> schemaStructs;
};

#endif // CODE_GENERATION_CODE_GENERATOR_H

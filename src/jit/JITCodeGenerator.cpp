#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

#include "jit/JITCodeGenerator.h"

JITCodeGenerator::JITCodeGenerator(Config &config, Schema &schema, ProfilingDataManager *profilingDataManager,
                                   CompileMode compileMode)
    : CodeGenerator(config, schema, compileMode) {
  this->profilingDataManager = profilingDataManager;
  open = CMethod::builder()
             .returns("void")
             .addParameter("GlobalState * g")
             .addParameter("Dispatcher * d")
             .addParameter("Variant * v")
             .withName("open");

  init = CMethod::builder()
             .returns("void")
             .addParameter("GlobalState * g")
             .addParameter("Dispatcher * d")
             .withName("init");

  execute =
      CMethod::builder().returns("void").addParameter("int threadID").addParameter("int numaNode").withName("execute");

  migrateFrom = CMethod::builder().returns("void").addParameter("void ** inputStates").withName("migrateFrom");

  migrateTo = CMethod::builder().returns("void").addParameter("void ** outputStates").withName("migrateTo");

  getState = CMethod::builder().returns("void **").addParameter("").withName("getState");

  open.addInstruction(CMethod::Instruction(INSTRUCTION_SYSTEM, "globalState = g;\n"));
  open.addInstruction(CMethod::Instruction(INSTRUCTION_SYSTEM, "dispatcher = d;\n"));
  open.addInstruction(CMethod::Instruction(INSTRUCTION_SYSTEM, "variant = v;\n"));
  getState.addInstruction(
      CMethod::Instruction(INSTRUCTION_SYSTEM, "void** statePtr = (void**)malloc(sizeof(void*)*2);\n"));
}

void JITCodeGenerator::generateStructFile(std::string path) {
  // Generate data_types.h file
  CFile::Builder dataTypesBuilder = CFile::builder().withName("data_types.h").include("tbb/atomic.h");

  // add schema structs
  size_t i = 0;
  for (auto &context : queryContexts) {
    if (context.stateStrategy == QueryContext::SHARED) {
      generateStruct(context.schema, "record", i, context.isAggregation);
    } else {
      generateStruct(context.schema, "record", i, false);
    }
    i++;
  }

  for (auto code : this->schemaStructs) {
    dataTypesBuilder.addCode(code);
  }

  // Generate data_types.h file
  CFile dataTypes = dataTypesBuilder.build();
  writeToFile(dataTypes);
}

CFile JITCodeGenerator::generate(std::string type, std::string path) {

  // Pipeline Permutation: permute longest pipeline if needed
  if (config.getPipelinePermutation() != 0) {
    CMethod::PipelineEnumerator enumerator = CMethod::PipelineEnumerator(pipelines[longestPipeline()]);
    enumerator.getPermutation(pipelines[longestPipeline()], config.getPipelinePermutation());
  }

  // Generate pipelines
  auto i = 0;
  for (auto &pipeline : pipelines) {
    CMethod pipelineMethod = pipeline.withName("pipeline" + std::to_string(i))
                                 .addParameter("int thread_id")
                                 .addParameter("int numa_node")
                                 .returns("void")
                                 .build();
    file.addMethod(pipelineMethod);
    i++;
  }

  std::stringstream executeFunction;
  auto startPipeline = this->pipelines.size() - 1;
  executeFunction << "while(dispatcher->hasWork() && variant->isValid()){\n"
                     "    void *records = dispatcher->getWork(threadID, 0);\n"

                     "     pipeline"
                  << startPipeline << "((record0 *) records, dispatcher->runLength, threadID, numaNode);\n";
  if (this->compileMode == CM_OPTIMIZE)
    executeFunction << "     variant->runtime->monitor(threadID);\n";

  executeFunction << "}\n";

  execute.addInstruction(CMethod::Instruction(INSTRUCTION_SYSTEM, executeFunction.str()));

  auto b = open.build();
  file.addMethod(b);

  auto in = init.build();
  file.addMethod(in);

  auto closeMethod = CMethod::builder().returns("void").withName("close");

  auto c = closeMethod.build();
  file.addMethod(c);

  auto m = migrateFrom.build();
  file.addMethod(m);

  auto t = migrateTo.build();
  file.addMethod(t);

  auto e = execute.build();
  file.addMethod(e);

  getState.addInstruction(CMethod::Instruction(INSTRUCTION_SYSTEM, "return statePtr;\n"));
  auto g = getState.build();
  file.addMethod(g);

  CFile queryFile = file.withName("query.cpp")
                        .include("data_types.h")
                        .include("runtime/JitDispatcher.h")
                        .include("runtime/jit_global_state.hpp")
                        .include("runtime/JitRuntime.h")
                        .include("runtime/Variant.hpp")
                        .include("tbb/atomic.h")
                        .addStatement("GlobalState * globalState;")
                        .addStatement("Variant * variant;")
                        .build();
  return queryFile;
}
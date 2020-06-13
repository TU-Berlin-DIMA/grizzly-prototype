#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

#include "code_generation/CodeGenerator.h"

#define GetCurrentDir getcwd

std::string GetCurrentWorkingDir(void) {
  char buff[FILENAME_MAX];
  GetCurrentDir(buff, FILENAME_MAX);
  std::string current_working_dir(buff);
  return current_working_dir;
}

std::string GENERATED_PATH;

CodeGenerator::CodeGenerator(Config &config, Schema &schema, CompileMode mode) : config(config), compileMode(mode) {

  std::string parallelism = std::to_string(config.getParallelism());
  std::string bufferSize = std::to_string(config.getBufferSize());
  schemaStructs = {};

  GENERATED_PATH = GetCurrentWorkingDir() + "/jit-generated-code/";
  std::cout << "Current working directory: " << GENERATED_PATH << std::endl;
  // initialize dispatcher
  // TODO:make type and path dynamic

  file.addStatement("static Dispatcher *dispatcher;");
  // start initial pipeline
  startPipeline();
}

CFile CodeGenerator::generate(std::string type, std::string path) {

  std::string parallelism = std::to_string(config.getParallelism());
  std::string bufferSize = std::to_string(config.getBufferSize());

  // Generate data_types.h file
  CFile::Builder dataTypesBuilder = CFile::builder().withName("data_types.h").include("tbb/atomic.h");

  // add schema structs
  size_t i = 0;
  for (auto &context : queryContexts) {
    generateStruct(context.schema, "record", i, context.isAggregation);
    i++;
  }

  for (auto code : this->schemaStructs) {
    dataTypesBuilder.addCode(code);
  }

  // Generate data_types.h file
  CFile dataTypes = dataTypesBuilder.build();
  writeToFile(dataTypes);

  // Generate code to run the query
  file.include("runtime/runtime.h");
  auto code = std::string("");
  // code += "auto file_path = std::string(argv[5]);\n";
  // code += "std::cout << \"runLength=\" <<  runLength << \" bufferSize=\" << buffer_size << \" dop=\" << parallelism
  // << \" runs=\" << runs << std::endl;\n"; code += "dispatcher= new Dispatcher(runLength, parallelism, buffer_size,
  // runs, 78, true);\n runtime::init(dispatcher);\n"; code += "dispatcher->setInput(" + type + ", file_path);\n"; code
  // += "runtime::run(query);\n";
  main.addInstruction(CMethod::Instruction(INSTRUCTION_SYSTEM, code));

  // Generate query method
  std::string maxPipeline = std::to_string(pipelines.size() - 1);

  std::stringstream statements;
  statements << "while(dispatcher->hasWork()) {" << std::endl;
  statements << "for(unsigned int currentBuffer = 0; currentBuffer < dispatcher->bufferRuns; currentBuffer++ ){"
             << std::endl;
  statements << "void* records = dispatcher->getWork(threadId,  currentBuffer);" << std::endl;
  if (config.getNuma()) {
    statements << "pipeline" << maxPipeline << "((record0 *) records, "
               << "dispatcher->runLength"
               << ",  threadId, dispatcher->numa_relation[threadId]);" << std::endl;
  } else {
    statements << "pipeline" << maxPipeline << "((record0 *) records, "
               << "dispatcher->runLength"
               << ",  threadId, 1);" << std::endl;
  }
  statements << "}" << std::endl;
  statements << "runtime::passes++;" << std::endl;
  statements << "}" << std::endl;

  CMethod::Builder query = CMethod::builder()
                               .withName("query")
                               .returns("void")
                               .addParameter("int threadId")
                               .addInstruction(CMethod::Instruction(INSTRUCTION_SYSTEM, statements.str()));

  CMethod queryMethod = query.build();

  // Generate main method
  CMethod mainMethod =
      main.withName("open").returns("void").addParameter("int buffer_size").addParameter("int parallelism").build();

  // Pipeline Permutation: permute longest pipeline if needed
  if (config.getPipelinePermutation() != 0) {
    CMethod::PipelineEnumerator enumerator = CMethod::PipelineEnumerator(pipelines[longestPipeline()]);
    enumerator.getPermutation(pipelines[longestPipeline()], config.getPipelinePermutation());
  }

  // Generate pipelines
  i = 0;
  for (auto &pipeline : pipelines) {
    CMethod pipelineMethod = pipeline.withName("pipeline" + std::to_string(i))
                                 .addParameter("int thread_id")
                                 .addParameter("int numa_node")
                                 .returns("void")
                                 .build();
    file.addMethod(pipelineMethod);
    i++;
  }

  CFile mainFile =
      file.withName("query.cpp").include("data_types.h").addMethod(queryMethod).addMethod(mainMethod).build();
  // writeToFile(mainFile);
  return mainFile;
}

void CodeGenerator::startPipeline() { pipelines.push_back(CMethod::builder()); }

size_t CodeGenerator::currentPipeline() { return pipelines.size() - 1; }

size_t CodeGenerator::longestPipeline() {
  size_t longest_pipeline = 0;
  for (size_t i = 0; i != pipelines.size(); ++i) {
    if (pipelines[i].instructions.size() > longest_pipeline) {
      longest_pipeline = i;
    }
  }
  return longest_pipeline;
}

CMethod::Builder &CodeGenerator::pipeline(size_t id) { return pipelines[id]; }

void CodeGenerator::addQueryContext(QueryContext ctx) {
  queryContexts.push_back(ctx);
  // queryContexts.push_back(ctx);
}

QueryContext &CodeGenerator::ctx(size_t id) { return queryContexts[id]; }

void CodeGenerator::compileCode() {
  // compile query
  std::cout << "Start compiling query" << std::endl;
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  std::string compile("clang++ -o0 -g -std=c++11 -I " + GENERATED_PATH + " " + GENERATED_PATH + "engine.cpp -o " +
                      GENERATED_PATH + "query " + " -pthread -ltbb -lnuma");
  std::cout << "run cmpString: " << compile << std::endl;
  system(compile.c_str());
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  double elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0;
  std::cout << "Compiled query in " + std::to_string(elapsed) + "s" << std::endl;
}

/*
 * compiles and runs the query
 */
void CodeGenerator::run() {
  // run query
  std::cout << "Running query" << std::endl << "-----------------" << std::endl;
  std::string run(GENERATED_PATH + "query");
  system(run.c_str());
}

/*
 * Generates a struct for the given schema
 */
CCode CodeGenerator::generateStruct(Schema &schema, std::string name, size_t id, bool useAtomic) {

  CCode::Builder builder =
      CCode::builder().addStatement("struct __attribute__((packed)) " + name + std::to_string(id) + "{");

  for (auto it = schema.fields.begin(); it != schema.fields.end(); ++it) {
    //    for (auto &mapEntry: schema.fields) {
    const Field &field = *it;
    std::string type = field.dataType.cType();
    if (useAtomic) {
      type = "tbb::atomic<" + type + ">";
    }

    if (field.size > 1) {
      builder.addStatement(type + " " + field.name + "[" + std::to_string(field.size) + "];");
    } else {
      builder.addStatement(type + " " + field.name + ";");
    }
  }

  builder.addStatement("};");
  auto code = builder.build();
  this->schemaStructs.push_back(code);
  return code;
}

void CodeGenerator::writeToFile(CFile &file) {
  std::string path = GENERATED_PATH + file.name;
  std::ofstream generated_file(path.c_str(), std::ios::trunc | std::ios::out);
  generated_file << file.output;
  // std::cout << file.output << std::endl;
  // std::cout << "writeToFile: " << path.c_str() << std::endl;
  generated_file.close();
}

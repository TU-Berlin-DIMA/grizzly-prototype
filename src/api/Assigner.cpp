#include <iostream>
#include <sstream>
#include <string>

#include "api/Assigner.h"

/*
 * Record can only be in exactly one window based on processing time
 */
void TumblingProcessingTimeAssigner::produce(CodeGenerator &cg) {
  pipeline = cg.currentPipeline();
  cg.file.addStatement("const int64_t window_size" + std::to_string(pipeline) + " = " + size.to_string() + ";");
  cg.file.addStatement("const int64_t window_buffers" + std::to_string(pipeline) + " = 2;");
  // save number of windows to query context
  cg.ctx(pipeline).numWindows = 2;
}

void TumblingProcessingTimeAssigner::consume(CodeGenerator &cg) {

  auto strPipeline = std::to_string(pipeline);
  std::string resultType = "record" + std::to_string(pipeline);

  // init state buffer for non-grouped query (resultType[2])
  if (!cg.ctx(pipeline).hasGroupBy) {
    cg.file.addStatement(resultType + "* state" + std::to_string(pipeline) + " = new " + resultType + "[2];");
  }

  std::stringstream statementsInit;
  // get key for keyed query
  std::string key = "";
  if (cg.ctx(pipeline).hasKeyBy)
    key = "[record." + cg.ctx(pipeline).keyBy->name + "]";

  statementsInit << "{g->window_state[" + strPipeline + "] = new WindowState{};\n"
                 << "auto window_state = g->window_state[" + strPipeline + "];\n"
                 << "window_state->thread_local_state = new ThreadLocalState*[dispatcher->parallelism];\n"
                 << "size_t ts = time(NULL);\n"
                 << "for (size_t thread_ID = 0; thread_ID < dispatcher->parallelism; thread_ID++) {\n";
  if (cg.config.getNuma()) {
    statementsInit << "int node = dispatcher->numa_relation[thread_ID];\n";
    statementsInit << "void* blob = numa_alloc_onnode((sizeof(runtime::ThreadLocalState)), node);\n";
    statementsInit << "window_state.thread_local_state[thread_ID] = new(blob) runtime::ThreadLocalState{};\n";
    statementsInit << "void* blob2 = numa_alloc_onnode((sizeof(int64_t)*window_buffers" << pipeline << "), node);\n";
    statementsInit << "window_state->thread_local_state[thread_ID]->windowEnds = new(blob2) int64_t[window_buffers"
                   << pipeline << "];\n";
  } else {
    statementsInit << "window_state->thread_local_state[thread_ID] = new ThreadLocalState{};\n";
    statementsInit << "window_state->thread_local_state[thread_ID]->windowEnds = new int64_t[window_buffers" << pipeline
                   << "];\n";
  }
  statementsInit << "for (size_t w = 0; w < window_buffers" << pipeline << "; w++) {;\n"
                 << "window_state->thread_local_state[thread_ID]->windowEnds[w] = ts + ( " << size.to_string()
                 << " * w) + " << size.to_string() << ";\n"
                 << "}"
                 << "}";
  statementsInit << "}";

  cg.init.addInstruction(CMethod::Instruction(INSTRUCTION_ASSIGNER, statementsInit.str()));
  // assign window
  std::string statements = "size_t window_index = thread_local_state->current_window;";
  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_ASSIGNER, statements));
}

/*
 * Record can belong to up to numWindows windows
 */
void SlidingProcessingTimeAssigner::produce(CodeGenerator &cg) {
  pipeline = cg.currentPipeline();

  cg.file.addStatement("const int64_t window_size" + std::to_string(pipeline) + " = " + size.to_string() + ";");
  cg.file.addStatement("const int64_t window_buffers" + std::to_string(pipeline) + " = " + std::to_string(numWindows) +
                       ";");

  // save number of windows to query context
  cg.ctx(pipeline).numWindows = numWindows;
}

void SlidingProcessingTimeAssigner::consume(CodeGenerator &cg) {
  std::stringstream statements_main;
  std::string resultType = "record" + std::to_string(pipeline);

  // init state-buffer for non-grouped query (resultType[1], size_t[1])
  if (!cg.ctx(pipeline).hasGroupBy) {

    cg.file.addStatement(resultType + "* state" + std::to_string(pipeline) + " = new " + resultType + "[" +
                         std::to_string(numWindows) + "];");

  } else {
    auto keyRange = cg.ctx(pipeline).maxKeyValue;
    if (keyRange != -1) {
      // cg.file.addStatement("record" + std::to_string(pipeline) + " **state" + std::to_string(pipeline) + ";");
      statements_main << "{"
                         " state"
                      << std::to_string(pipeline) << " = new record" + std::to_string(pipeline) + " *[window_buffers"
                      << std::to_string(pipeline) << " * " << cg.config.getNumaNodes() << "];"
                      << "for (size_t w = 0; w < (window_buffers" << std::to_string(pipeline) << " * "
                      << cg.config.getNumaNodes() << " ) ; w++) {";
      statements_main << "state" << std::to_string(pipeline) << "[w] = new record" + std::to_string(pipeline) + "["
                      << keyRange << " + 1];";
      statements_main << " for (size_t i = 0; i < " << keyRange << " + 1; i++) {";
      statements_main << "state" << std::to_string(pipeline) << "[w][i] = {};";
      statements_main << " }"
                         " }"
                      << "}";
    } else {
      // init state-buffer buffer for grouped-query (Map<keyType,resultType>[1], size_t[1])
      /*
            std::string keyType = cg.ctx(pipeline).groupBy->dataType.keyType();
            cg.file.addStatement("tbb::concurrent_unordered_map<" + keyType + ", " + resultType + ">* state" +
                                 std::to_string(pipeline) + " = new tbb::concurrent_unordered_map<" + keyType + ", " +
                                 resultType + ">[" + std::to_string(numWindows) + "];");
                                 */
    }
  }

  std::stringstream statements;
  std::stringstream statements_final;
  statements << "// ASSIGNER \n  // place tuple in correct window and hash key bucket \n"
             << "for(size_t w=0;w<" << std::to_string(numWindows) << ";w++) { \n"
             << "size_t window_index = (thread_local_state->current_window + w) % " << std::to_string(numWindows)
             << "; \n"
             << "if ((thread_local_state->windowEnds[window_index] - window_size" << std::to_string(pipeline)
             << ") <= ts) {";
  // cout << "add to windows window_index " << window_index << " window_start " << window_start <<  " ts " << ts  <<
  // endl;

  statements_final << "}" << std::endl;
  statements_final << "}" << std::endl;
  cg.pipeline(pipeline).addInstruction(
      CMethod::Instruction(INSTRUCTION_ASSIGNER, statements.str(), statements_final.str()));

  statements_main << "{g->window_state[" << pipeline << "] = new WindowState{};\n"
                  << "auto window_state = g->window_state[" << pipeline << "];\n"
                  << "window_state->thread_local_state = new ThreadLocalState*[dispatcher->parallelism];\n"
                  << "size_t ts = time(NULL);\n"
                  << "for (size_t thread_ID = 0; thread_ID < dispatcher->parallelism; thread_ID++) {\n";
  statements_main << "window_state->thread_local_state[thread_ID] = new ThreadLocalState{};\n";
  statements_main << "window_state->thread_local_state[thread_ID]->windowEnds = new int64_t[window_buffers" << pipeline
                  << "];\n";
  statements_main << "for (size_t w = 0; w < window_buffers" << pipeline << "; w++) {;\n"
                  << "window_state->thread_local_state[thread_ID]->windowEnds[w] = ts + ( " << slide.to_string()
                  << " * w) + " << size.to_string() << ";\n"
                  << "}"
                  << "}";
  statements_main << "}";

  cg.init.addInstruction(CMethod::Instruction(INSTRUCTION_ASSIGNER, statements_main.str()));
}

/*
 * Record can only be in exactly one window based on processing time
 */
void SessionProcessingTimeAssigner::produce(CodeGenerator &cg) {
  pipeline = cg.currentPipeline();

  // save number of windows to query context
  cg.ctx(pipeline).numWindows = 2;
}

void SessionProcessingTimeAssigner::consume(CodeGenerator &cg) {

  std::string resultType = "record" + std::to_string(pipeline - 1);

  // init state buffer for non-grouped query (resultType[2])
  if (!cg.ctx(pipeline).hasGroupBy) {
    cg.file.addStatement(resultType + "* state = new " + resultType + "[2];");
  }

  // init state-buffer buffer for grouped-query (Map<keyType,resultType>[2])
  if (cg.ctx(pipeline).hasGroupBy) {
    std::string keyType = cg.ctx(pipeline).groupBy->dataType.keyType();
    cg.file.addStatement("tbb::concurrent_unordered_map<" + keyType + ", " + resultType +
                         ">* state = new tbb::concurrent_unordered_map<" + keyType + ", " + resultType + ">[2];");
  }

  // get key for keyed query
  std::string key = "";
  if (cg.ctx(pipeline).hasKeyBy)
    key = "[record." + cg.ctx(pipeline).keyBy->name + "]";

  std::stringstream statements;

  // assign window
  statements << "size_t window = triggerCount" << key << " % 2;" << std::endl;

  // update timeout
  statements << "meta[window]" << key << " = ts + " << std::to_string(timeout.time) << ";" << std::endl;

  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_ASSIGNER, statements.str()));
}

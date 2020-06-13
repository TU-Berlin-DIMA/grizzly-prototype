#include <iostream>
#include <sstream>
#include <string>

#include "api/Trigger.h"

void CountTrigger::onBeforeElement(CodeGenerator &cg, size_t pipeline) {

  std::stringstream statements;
  std::string maxCountS = std::to_string(maxCount);
  std::string numWindows = std::to_string(cg.ctx(pipeline).numWindows);

  // trigger meta-data (count per window)
  std::string key = "";
  if (cg.ctx(pipeline).hasKeyBy) {
    if (cg.ctx(pipeline).maxKeyValue != -1) {
      cg.file.addStatement("tbb::concurrent_unordered_map<std::string,tbb::atomic<size_t>> meta[" + numWindows + "];");
      // cg.file.addStatement("tbb::concurrent_unordered_map<std::string,tbb::atomic<size_t>> triggerCount;");
      key = "[record." + cg.ctx(pipeline).keyBy->name + "]";
    } else {
      cg.file.addStatement("tbb::concurrent_unordered_map<std::string,tbb::atomic<size_t>> meta[" + numWindows + "];");
      // cg.file.addStatement("tbb::concurrent_unordered_map<std::string,tbb::atomic<size_t>> triggerCount;");
      key = "[record." + cg.ctx(pipeline).keyBy->name + "]";
    }
  } else {
    cg.file.addStatement("tbb::atomic<size_t> meta[" + numWindows + "];");
    cg.file.addStatement("tbb::atomic<size_t> triggerCount;");
  }

  // trigger condition
  statements << "size_t count = meta[window]" + key + ".fetch_and_increment();" << std::endl;
  statements << "if(count == " + maxCountS + ") {" << std::endl;
  // statements << "triggerCount" + key + "++;" << std::endl;
  statements << "pipeline" + std::to_string(pipeline - 1) + "(state[window]" + key + ");" << std::endl;

  if (this->purge) {
    if (cg.ctx(pipeline).hasGroupBy && !cg.ctx(pipeline).hasKeyBy) {
      statements << "state[window].clear();" << std::endl;
    } else {
      statements << "state[window]" + key + " = {};" << std::endl;
    }
  }
  statements << "meta[window]" + key + " = 0;" << std::endl;
  statements << "}" << std::endl;

  // re-run loop, if window triggered
  statements << "if(count >= " + maxCountS + ") {" << std::endl;
  statements << "i--; continue;" << std::endl;
  statements << "}" << std::endl;

  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_TRIGGER, statements.str()));
}

void ProcessingTimeTrigger::onBeforeAssign(CodeGenerator &cg, size_t pipeline) {

  std::stringstream statements;
  std::stringstream statements_main;
  std::string interval = std::to_string(every.time);
  std::string numWindows = std::to_string(cg.ctx(pipeline).numWindows);

  // trigger meta-data (timestamp, when each window needs to be triggered)
  std::string key = "";

  statements << "int64_t ts = time(NULL);\n" << std::endl;
  statements << "if (ts >= thread_local_state->windowEnds[thread_local_state->current_window]) {"
             << "size_t old_window = thread_local_state->current_window;\n"
             << "// change the window state of this thread -> so from now on it will put tuple to the next window \n"
             << "thread_local_state->windowEnds[old_window] += (window_size" << pipeline << " * window_buffers"
             << pipeline << ");\n"
             << "thread_local_state->current_window = (old_window + 1) % window_buffers" << pipeline << ";\n"
             << "int64_t oldCount = window_state->global_tigger_counter.fetch_and_increment();\n";

  statements << "if (oldCount == dispatcher->parallelism-1) {"
             << "window_state->global_tigger_counter = 0;\n";
  cg.ctx(pipeline);

  if (cg.ctx(pipeline).hasGroupBy) {
    if (cg.config.getNuma()) {
      statements << "//merge local states \n";
      statements << " for (int b = 0; b < " << cg.ctx(pipeline).maxKeyValue
                 << "; b++) {\n"
                    "  state"
                 << (pipeline) << "[old_window][b].count += state" << (pipeline)
                 << "[old_window + 2][b].count;\n"
                    "}";
    }

    if (cg.ctx(pipeline).maxKeyValue != -1) {

      statements << "pipeline" << (pipeline - 1) << "(old_window,thread_id, numa_node);\n";
      statements << "record" << (pipeline) << " t;";
      statements << "std::fill(state" << pipeline << "[old_window],"
                 << "state" << pipeline << "[old_window] + " << cg.ctx(pipeline).maxKeyValue << ", t);";
    } else {
      statements << "pipeline" << (pipeline - 1) << "(state" << pipeline << "[old_window],thread_id, numa_node);\n";

      statements << "state" << pipeline << "[old_window].clear();\n";
    }
  } else {
    statements << "pipeline" << (pipeline - 1) << "(state" << (pipeline) << "[old_window],thread_id, numa_node);\n";
    statements << "state" << pipeline << "[old_window] = {};\n";
  }

  statements << "}}";

  cg.main.addInstruction(CMethod::Instruction(INSTRUCTION_TRIGGER, statements_main.str()));
  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_TRIGGER, statements.str()));
}

void PurgingTrigger::onBeforeElement(CodeGenerator &cg, size_t pipeline) {
  trigger->purge = true;
  trigger->onBeforeElement(cg, pipeline);
}

void PurgingTrigger::onBeforeAssign(CodeGenerator &cg, size_t pipeline) {
  trigger->purge = true;
  trigger->onBeforeAssign(cg, pipeline);
}

#include <iostream>
#include <sstream>
#include <string>

#include "operator/WindowOperator.h"

WindowOperator::WindowOperator(Assigner *assigner, Trigger *trigger, Operator *input)
    : assigner(assigner), trigger(trigger), input(input) {
  leftChild = NULL;
  rightChild = NULL;
  input->parent = this;
  name = "Window";
}

std::string WindowOperator::to_string() { return "Window " + assigner->to_string() + " " + trigger->to_string(); }

WindowOperator::~WindowOperator() { delete input; }

void WindowOperator::consume(CodeGenerator &cg) {
  QueryContext &context = cg.ctx(pipeline);
  if (context.hasGroupBy && cg.compileMode == CM_OPTIMIZE && cg.config.distributionOpt()) {

    auto distribution = cg.profilingDataManager->getDistributionProfilingHandler("dist");
    auto top = distribution->top;
    auto freq = distribution->freq;

    int64_t min = INT64_MAX;
    int64_t max = INT64_MIN;
    int64_t sumFreq = 0;
    for (auto it = top.begin(); it < top.end() - 1; it++) {
      auto t = *it;
      if (min > t)
        min = t;

      if (max < t)
        max = t;
      sumFreq += freq[t];
      std::cout << t << ":" << freq[t] << std::endl;
    }

    auto freqAfg = ((double)sumFreq) / ((double)top.size());

    double sum_STD = 0;
    for (auto t = top.begin(); t < top.end() - 1; t++) {
      sum_STD += std::pow(freq[*t] - freqAfg, 2.0);
    }
    double std = std::sqrt(sum_STD);

    std::cout << "Min i: " << min << " Max i:" << max << " STE:" << std << std::endl;

    if (std < 10000) {
      context.stateStrategy = QueryContext::SHARED;
      std::cerr << " USE SHARED STATE " << std::endl;
    } else {
      context.stateStrategy = QueryContext::INDEPENDENT;
      std::cerr << " USE INDEPENDENT STATE " << std::endl;
    }

  } else {
    context.stateStrategy = QueryContext::SHARED;
  }

  std::cout << " pipeline " << pipeline << " " << to_string() << std::endl;

  std::stringstream statements;
  statements << "auto window_state = globalState->window_state[" << pipeline << "];\n";
  statements << "ThreadLocalState *thread_local_state = window_state->thread_local_state[thread_id];\n";
  cg.pipeline(pipeline).prependInstruction(CMethod::Instruction(INSTRUCTION_TRIGGER, statements.str()));

  // trigger before assign
  trigger->onBeforeAssign(cg, pipeline);

  // assign
  assigner->consume(cg);

  // trigger before element
  trigger->onBeforeElement(cg, pipeline);

  if (parent != nullptr) {
    parent->consume(cg);
  }
}

void WindowOperator::produce(CodeGenerator &cg) {

  pipeline = cg.currentPipeline();
  //  std::cout<< " pipeline " << pipeline << " " << to_string() << std::endl;
  assigner->produce(cg);
  input->produce(cg);
}

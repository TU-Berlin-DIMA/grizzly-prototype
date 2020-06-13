#include <iomanip>
#include <iostream>
#include <jit/JITCodeGenerator.h>
#include <jit/runtime/JitDispatcher.h>
#include <jit/runtime/Variant.hpp>
#include <jit/runtime/jit_global_state.hpp>
#include <operator/FinalWindowAggOperator.h>
#include <operator/MapOperator.h>
#include <operator/PrintOperator.h>
#include <operator/SelectOperator.h>
#include <operator/WriteOperator.h>
#include <operator/WriteToMemOperator.h>
#include <thread>

#include "api/Query.h"
#include "code_generation/CodeGenerator.h"
#include "jit/CodeCompiler.hpp"
#include "jit/JITExecutionRuntime.h"
#include "operator/AggregateOperator.h"
#include "operator/FilterOperator.h"
#include "operator/GroupByOperator.h"
#include "operator/InputOperator.h"
#include "operator/KeyOperator.h"
#include "operator/MapOperator.h"
#include "operator/PrintOperator.h"
#include "operator/ReadOperator.h"
#include "operator/ReadWindowOperator.h"
#include "operator/SelectOperator.h"
#include "operator/WindowOperator.h"
#include "operator/WriteOperator.h"
#include "tbb/concurrent_unordered_map.h"

Query::Query(Config &config, Schema &schema) : schema(schema), config(config) { current = NULL; }

Query::~Query() {}

Query Query::generate(Config &config, Schema &schema, std::string path) {
  Query *q = new Query(config, schema);
  config.setSourceFile(path);
  q->current = new InputOperator(BinaryFile, path, new ReadOperator(schema));
  //	q->current = new InputOperator(type, path);
  q->root = q->current;
  //    ReadOperator* readOp = new ReadOperator(schema);
  //    readOp->parent = q->root;
  return *q;
}

void Query::generate() {
  CodeGenerator codeGenerator = CodeGenerator(config, schema, CM_DEFAULT);
  QueryContext queryContext = QueryContext(schema);
  codeGenerator.addQueryContext(queryContext);
  current->produce(codeGenerator);
  Operator *input = root;
  while (input->rightChild) {
    input = input->rightChild;
  }
  InputOperator *inputOp = (InputOperator *)input;
  auto file = codeGenerator.generate(inputOp->getInputTypeAsString(), inputOp->getPath());
  codeGenerator.writeToFile(file);
  codeGenerator.compileCode();
}

void Query::execute() {
  auto jitExecutionRuntime = new JITExecutionRuntime();
  jitExecutionRuntime->execute(this);
}

/*
 * Relational Operators
 */
Query &Query::filter(Predicate &predicate) {
  Operator *newOp = new FilterOperator(predicate, current);
  if (current)
    newOp->rightChild = current;
  root = newOp;
  current = newOp;
  return *this;
}

Query &Query::select(std::vector<std::string> fields) {
  Operator *newOp = new SelectOperator(current, fields);
  if (current)
    newOp->rightChild = current;
  root = newOp;
  current = newOp;
  return *this;
}

Query &Query::groupBy(std::string fieldId, int maxValue) {
  Operator *newOp = new GroupByOperator(schema.get(fieldId), current, maxValue);
  if (current)
    newOp->rightChild = current;
  root = newOp;
  current = newOp;
  return *this;
}

Query &Query::groupBy(std::string fieldId) {
  Operator *newOp = new GroupByOperator(schema.get(fieldId), current);
  if (current)
    newOp->rightChild = current;
  root = newOp;
  current = newOp;
  return *this;
}

Query &Query::aggregate(Aggregation &&aggregation) {
  // TODO: diff between window and batch
  Operator *newOp = new ReadWindowOperator(schema, new AggregateOperator(aggregation, current));
  if (current)
    newOp->rightChild = current;
  root = newOp;
  current = newOp;
  if (aggregation.hasFinalAggregation()) {
    Operator *newOp2 = new FinalWindowAggOperator(&aggregation, current);
    newOp2->rightChild = current;
    root = newOp2;
    current = newOp2;
  }

  return *this;
}

/*
 * Streaming Operators
 */
Query &Query::window(Window &&window) {
  Operator *newOp = new WindowOperator(window.assigner, window.trigger, current);
  if (current)
    newOp->rightChild = current;
  root = newOp;
  current = newOp;
  return *this;
}

Query &Query::map(Mapper &&mapper) {
  Operator *newOp = new MapOperator(mapper, current);
  if (current)
    newOp->rightChild = current;
  root = newOp;
  current = newOp;
  return *this;
}

/*
 * Input Operators
 */
Query &Query::input(InputType type, std::string path) {
  assert(0);
  Operator *newOp = new InputOperator(type, path, current);
  if (current)
    newOp->rightChild = current;
  root = newOp;
  current = newOp;
  return *this;
}

/*
 * Output Operators
 */
Query &Query::write(std::string fileName) {
  Operator *newOp = new WriteOperator(fileName, current);
  if (current)
    newOp->rightChild = current;
  root = newOp;
  current = newOp;
  return *this;
}

Query &Query::print() {
  Operator *newOp = new PrintOperator(current);
  if (current)
    newOp->rightChild = current;
  root = newOp;
  current = newOp;
  return *this;
}

Query &Query::toOutputBuffer() {
  Operator *newOp = new WriteToMemOperator(current);
  if (current)
    newOp->rightChild = current;
  root = newOp;
  current = newOp;
  return *this;
}

void Query::printQueryPlan(Operator *p, int indent) {
  // Taken from https://stackoverflow.com/questions/13484943/print-a-binary-tree-in-a-pretty-way

  if (p != NULL) {
    if (p->rightChild) {
      printQueryPlan(p->rightChild, indent + 4);
    }
    if (indent) {
      std::cout << std::setw(indent) << ' ';
    }
    if (p->rightChild)
      std::cout << " /\n" << std::setw(indent) << ' ';
    std::cout << p->to_string() << "\n ";
    if (p->leftChild) {
      std::cout << std::setw(indent) << ' ' << " \\\n";
      printQueryPlan(p->leftChild, indent + 4);
    }
  }
}

void Query::printQueryPlan(Query query) {
  std::cout << "Query Plan " << std::string(69, '-') << std::endl;

  if (query.root == NULL) {
    printf("No root node; cant print queryplan\n");
  } else {
    printQueryPlan(query.current, 0);
    printf("\n");
  }
}

void Query::printPipelinePermutations(Query query) {
  std::cout << "Query Plan - Permutations of the longest Pipeline " << std::string(30, '-') << std::endl;

  /* Produce Code Generator */
  CodeGenerator code_generator = CodeGenerator(query.config, query.schema, CM_DEFAULT);
  QueryContext query_context = QueryContext(query.schema);
  code_generator.addQueryContext(query_context);
  query.current->produce(code_generator);

  /* Choose longest Pipeline and get enumerator. */
  CMethod::Builder longest_pipeline = code_generator.pipeline(code_generator.longestPipeline());
  CMethod::PipelineEnumerator enumerator = CMethod::PipelineEnumerator(longest_pipeline);

  /* Print all permutations. */
  enumerator.printPermutations();
  std::cout << std::endl;
}
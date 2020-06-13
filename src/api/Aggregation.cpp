#include <iostream>
#include <operator/WindowOperator.h>
#include <sstream>
#include <string>

#include "api/Aggregation.h"

void Aggregation::produce_(CodeGenerator &cg, Operator *input, Schema &schema) {

  // start new pipeline
  QueryContext context = QueryContext(schema);
  cg.addQueryContext(context);
  cg.startPipeline();

  // get current pipeline id
  pipeline = cg.currentPipeline();

  // set aggregation in query context
  cg.ctx(pipeline).isAggregation = true;
  input->produce(cg);
}

void Aggregation::consume_(CodeGenerator &cg, Operator *input) {
  std::stringstream statements;
  std::string key;

  if (cg.ctx(pipeline).hasGroupBy) {
    statements << "auto keyField = record." + cg.ctx(pipeline).groupBy->name << ";" << std::endl;
    if (cg.compileMode == CM_DEFAULT || cg.compileMode == CM_INSTRUMENT) {
      statements << "auto key = keyField;\n";
    } else if (cg.compileMode == CM_OPTIMIZE) {
      auto profiledMax = cg.profilingDataManager->getMaxHandler("agg_max")->getValue();
      auto profiledMin = cg.profilingDataManager->getMaxHandler("agg_min")->getValue();
      auto top = cg.profilingDataManager->getDistributionProfilingHandler("dist")->top;
      std::string keyType = cg.ctx(pipeline).groupBy->dataType.keyType();
      if (keyType == "std::string") {
        statements << "uint64_t value =  (uint64_t) fast_atoi(keyField);\n";
      } else {
        statements << "uint64_t value =  (uint64_t) keyField;\n";
      }
      statements << "if(value< " << profiledMin << " || value>" << profiledMax << "){throw DeoptimizeException("
                 << pipeline
                 << ",i, records);}\n"
                    "uint64_t key = value % "
                 << profiledMax << ";\n";
    }

    if (cg.compileMode == CM_INSTRUMENT) {
      cg.profilingDataManager->registerMinHandler("agg_min");
      cg.profilingDataManager->registerMaxHandler("agg_max");
      cg.profilingDataManager->registerDistributionHandler("dist");
      statements << "if(thread_id==0){";
      std::string keyType = cg.ctx(pipeline).groupBy->dataType.keyType();
      if (keyType == "std::string") {
        statements << "uint64_t intKey =  (uint64_t) atoi(keyField);\n";
      } else {
        statements << "uint64_t intKey =  (uint64_t) keyField;\n";
      }
      statements << "variant->profilingDataManager->getMinHandler(\"agg_min\")->update(intKey);\n"
                    "variant->profilingDataManager->getMaxHandler(\"agg_max\")->update(intKey);\n"
                    "variant->profilingDataManager->getDistributionProfilingHandler(\"dist\")->update(intKey);}\n";
    }
  }

  // get window buffer

  auto strPipeline = std::to_string(pipeline);

  if (cg.compileMode == CM_OPTIMIZE) {
    auto &context = cg.ctx(pipeline);
    if (context.stateStrategy == QueryContext::INDEPENDENT && !cg.config.getNuma()) {
      statements << "auto bufferIndex = window_index + (thread_id * window_buffers" << strPipeline << ");";
    } else if (context.stateStrategy == QueryContext::SHARED && cg.config.getNuma()) {
      statements << "auto bufferIndex = window_index + (numa_node * window_buffers" << strPipeline << ");";
    } else {
      statements << "auto bufferIndex = window_index;";
    }
  } else {
    statements << "auto bufferIndex = window_index;";
  }

  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_AGGREGATE, statements.str()));
}

void Aggregation::createState(CodeGenerator &cg, Operator *input, Schema &schema) {
  auto strPipeline = std::to_string(pipeline);
  auto &contex = cg.ctx(pipeline);
  auto resultType = "record" + std::to_string(pipeline);
  std::stringstream statementsOpen;
  statementsOpen << "{";
  // init state-buffer buffer for grouped-query (Map<keyType,resultType>[2])
  if (cg.ctx(pipeline).hasGroupBy) {
    auto keyRange = cg.ctx(pipeline).maxKeyValue;
    if (cg.compileMode == CM_OPTIMIZE && keyRange != -1) {
      cg.file.addStatement("record" + strPipeline + " **state" + strPipeline + ";");

      if (contex.stateStrategy == QueryContext::INDEPENDENT) {
        statementsOpen << "// init state buffer for independent aggregation\n"
                          " auto stateBuffers = window_buffers"
                       << strPipeline << " * dispatcher->parallelism * " << cg.config.getNumaNodes() << ";\n";
      } else {
        statementsOpen << "// init state buffer for shared aggregation\n"
                          " auto stateBuffers = window_buffers"
                       << strPipeline << " * " << cg.config.getNumaNodes() << ";\n";
      }

      statementsOpen << " state" << strPipeline << " = new " + resultType + " *[stateBuffers];"
                     << "for (size_t w = 0; w < (stateBuffers) ; w++) {\n";

      if (cg.config.getNuma()) {
        statementsOpen << "void *blob = numa_alloc_onnode((sizeof(record" + strPipeline + ") * " << keyRange
                       << "+1) , w / " << cg.config.getNumaNodes() << ");\n"
                       << "state" << strPipeline << "[w]  = new(blob) record" + strPipeline + "[" << keyRange
                       << " + 1];";
      } else {
        statementsOpen << "state" << strPipeline
                       << "[w] = "
                          "new record" +
                              strPipeline + "["
                       << keyRange << " + 1];";
      }

      statementsOpen << " for (size_t i = 0; i < " << keyRange << " + 1; i++) {";
      statementsOpen << "state" << strPipeline << "[w][i] = {};";
      statementsOpen << " }"
                        " }";

    } else {
      std::string keyType = cg.ctx(pipeline).groupBy->dataType.keyType();
      cg.file.addStatement("tbb::concurrent_unordered_map<" + keyType + ", " + resultType + ">* state" + strPipeline +
                           ";\n");
      statementsOpen << "state" << strPipeline
                     << " = new tbb::concurrent_unordered_map<" + keyType + ", " + resultType + ">[" +
                            std::to_string(2 * cg.config.getNumaNodes() * cg.ctx(pipeline).numWindows) + "];";
    }
  }
  statementsOpen << "}";

  cg.open.addInstruction(CMethod::Instruction(INSTRUCTION_ASSIGNER, statementsOpen.str()));
}

void Aggregation::migrateFrom(CodeGenerator &cg, Operator *input, Schema &schema) {

  auto strPipeline = std::to_string(pipeline);
  std::string resultType = "record" + std::to_string(pipeline);
  std::stringstream statementsMigrateFrom;
  statementsMigrateFrom << "{";

  // init state-buffer buffer for grouped-query (Map<keyType,resultType>[2])
  if (cg.ctx(pipeline).hasGroupBy) {
    std::string keyType = cg.ctx(pipeline).groupBy->dataType.keyType();
    statementsMigrateFrom << "tbb::concurrent_unordered_map<" << keyType
                          << ", record1>* input = ((tbb::concurrent_unordered_map<" << keyType
                          << ", record1>*)inputStates[" << strPipeline << "]);\n";
    statementsMigrateFrom << "for (size_t w = 0; w < (window_buffers" << strPipeline << " ) ; w++) {\n"
                          << " for (size_t n = 0; n < " << cg.config.getNumaNodes() << " ; n++){\n"
                          << "  for (auto const &it : input[w]) {\n";
    statementsMigrateFrom << keyType << " key = it.first;" << std::endl;
    statementsMigrateFrom << resultType << " record = it.second;" << std::endl;
    if (cg.ctx(pipeline).maxKeyValue != -1) {
      if (keyType == "string") {
        statementsMigrateFrom << "auto keyValue = ((uint64_t) key.data());" << std::endl;
      } else {
        statementsMigrateFrom << " auto keyValue = ((uint64_t) key);" << std::endl;
      }
      statementsMigrateFrom << "uint64_t keyInt = keyValue % " << cg.ctx(pipeline).maxKeyValue << ";\n";
      for (auto field : schema.fields) {
        auto state = "state" + strPipeline + "[w+n][keyInt]." + field.name;
        statementsMigrateFrom << state << " = " + state + " + record." << field.name << ";\n";
      }

    } else {
      for (auto field : schema.fields) {
        auto state = "state" + strPipeline + "[w+n][key]." + field.name;
        statementsMigrateFrom << state << " = " + state + " + record." << field.name << ";\n";
      }
    }
    statementsMigrateFrom << " }}}";
  }

  statementsMigrateFrom << "}";

  cg.migrateFrom.addInstruction(CMethod::Instruction(INSTRUCTION_ASSIGNER, statementsMigrateFrom.str()));
}

void Aggregation::migrateTo(CodeGenerator &cg, Operator *input, Schema &schema) {

  auto strPipeline = std::to_string(pipeline);
  auto contex = cg.ctx(pipeline);
  std::string resultType = "record" + std::to_string(pipeline);
  std::stringstream statementsMigrateFrom;
  statementsMigrateFrom << "{";

  // init state-buffer buffer for grouped-query (Map<keyType,resultType>[2])
  if (cg.ctx(pipeline).hasGroupBy) {
    std::string keyType = cg.ctx(pipeline).groupBy->dataType.keyType();
    statementsMigrateFrom << "tbb::concurrent_unordered_map<" << keyType
                          << ", record1>* output = ((tbb::concurrent_unordered_map<" << keyType
                          << ", record1>*)outputStates[" << strPipeline << "]);\n";
    statementsMigrateFrom << "for (size_t w = 0; w < (window_buffers" << strPipeline << " ) ; w++) {\n"
                          << " for (size_t n = 0; n < " << cg.config.getNumaNodes() << " ; n++){\n";

    if (contex.maxKeyValue != -1) {
      statementsMigrateFrom << "for (int i = 0; i<  " << cg.ctx(pipeline).maxKeyValue << ";i++ ) {\n";
      if (keyType == "string") {
        statementsMigrateFrom << keyType << " key = std::to_string(i);" << std::endl;
      } else {
        statementsMigrateFrom << keyType << " key = i;" << std::endl;
      }
      statementsMigrateFrom << resultType << " record = state" << strPipeline << "[w*n][i];" << std::endl;
      for (auto field : schema.fields) {
        auto state = "output[w][key]." + field.name;
        statementsMigrateFrom << state << " = " << state << " + record." << field.name << ";\n";
      }
      statementsMigrateFrom << "}";
    } else {
      statementsMigrateFrom << "  for (auto const &it : state" << strPipeline << "[w*n]) {\n";
      statementsMigrateFrom << keyType << " key = it.first;" << std::endl;
      statementsMigrateFrom << resultType << " record = it.second;" << std::endl;
      for (auto field : schema.fields) {
        auto state = "output[w][key]." + field.name;
        statementsMigrateFrom << state << " = " << state << " + record." << field.name << ";\n";
      }
      statementsMigrateFrom << " }";
    }
    statementsMigrateFrom << " }}";
  }

  statementsMigrateFrom << "}";

  cg.migrateTo.addInstruction(CMethod::Instruction(INSTRUCTION_ASSIGNER, statementsMigrateFrom.str()));
}

void Aggregation::addStatePtr(CodeGenerator &cg, Operator *input, Schema &schema) {
  auto strPipeline = std::to_string(pipeline);
  std::string resultType = "record" + std::to_string(pipeline);
  std::stringstream statementsMigrateFrom;
  if (cg.ctx(pipeline).maxKeyValue == -1) {
    statementsMigrateFrom << "auto output = state" << strPipeline << ";";
    statementsMigrateFrom << "statePtr[" << strPipeline << "] = state" << strPipeline << ";\n";
  }
  cg.getState.addInstruction(CMethod::Instruction(INSTRUCTION_ASSIGNER, statementsMigrateFrom.str()));
}

void Aggregation::consumeFinalAggregation(CodeGenerator &generator, Operator *pOperator) {}
void Aggregation::produceFinalAggregation(CodeGenerator &generator, Operator *pOperator) {}

/*
 * SUM
 */
void Sum::produce(CodeGenerator &cg, Operator *input) {
  // add field to schema
  Schema schema = Schema::create().addFixSizeField(fieldId + "_sum", DataType::Long, Stream);
  produce_(cg, input, schema);
  createState(cg, input, schema);
  migrateFrom(cg, input, schema);
  migrateTo(cg, input, schema);
  addStatePtr(cg, input, schema);
}

void Sum::consume(CodeGenerator &cg, Operator *parent) {

  std::stringstream statements;

  // get key
  consume_(cg, parent);

  // calculate sum
  auto sumField = fieldId + "_sum";
  if (cg.config.getNuma()) {
    statements << "state" << std::to_string(pipeline) << "[bufferIndex][key]." << sumField << " += record." << fieldId
               << ";";
  } else {
    if (cg.ctx(pipeline).hasGroupBy) {
      statements << "state" << std::to_string(pipeline) << "[bufferIndex][key]." << sumField << " += record." << fieldId
                 << ";";
    } else {
      statements << "state" << std::to_string(pipeline) << "[bufferIndex]." << sumField << " += record." << fieldId
                 << ";";
    }
  }
  // calculate count
  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_AGGREGATE, statements.str()));

  if (parent != nullptr) {
    parent->consume(cg);
  }
}

/*
 * COUNT
 */
void Count::produce(CodeGenerator &cg, Operator *input) {
  // add field to schema
  Schema schema = Schema::create().addFixSizeField("count", DataType::Long, Stream);

  produce_(cg, input, schema);
  createState(cg, input, schema);
  migrateFrom(cg, input, schema);
  migrateTo(cg, input, schema);
  addStatePtr(cg, input, schema);
}

void Count::consume(CodeGenerator &cg, Operator *parent) {

  std::stringstream statements;
  std::string key;

  // get key
  consume_(cg, parent);

  // increment
  if (cg.config.getNuma()) {
    statements << "state" << std::to_string(pipeline) << "[bufferIndex][key].count++;";
  } else {
    if (cg.ctx(pipeline).hasGroupBy) {
      statements << "state" + std::to_string(pipeline) + "[bufferIndex][key].count++;" << std::endl;
    } else {
      statements << "state" + std::to_string(pipeline) + "[bufferIndex].count++;" << std::endl;
    }
  }
  // calculate count
  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_AGGREGATE, statements.str()));

  if (parent != nullptr) {
    parent->consume(cg);
  }
}

/*
 * Min
 */
void Min::produce(CodeGenerator &cg, Operator *input) {
  // add field to schema
  Schema schema = Schema::create().addFixSizeField(fieldId + "_min", DataType::Long, Stream);
  produce_(cg, input, schema);
  createState(cg, input, schema);
  migrateFrom(cg, input, schema);
  migrateTo(cg, input, schema);
  addStatePtr(cg, input, schema);
}

void Min::consume(CodeGenerator &cg, Operator *parent) {

  std::string key;
  /* if (cg.ctx(pipeline).hasGroupBy)
     key = "[record." + cg.ctx(pipeline).groupBy->name + "]";
 */
  // calculate min
  std::stringstream statements;

  consume_(cg, parent);

  statements << "auto recordValue = record." << fieldId << "; ";

  std::stringstream oldValue;
  if (cg.config.getNuma()) {
    oldValue << "state" << std::to_string(pipeline) << "[bufferIndex][key]." << fieldId << "_min";
  } else {
    if (cg.ctx(pipeline).hasGroupBy) {
      oldValue << "state" << std::to_string(pipeline) << "[bufferIndex][key]." << fieldId << "_min";
    } else {
      oldValue << "state" << std::to_string(pipeline) << "[bufferIndex]." << fieldId << "_min";
    }
  }
  statements << "long old; do {\n"
                "// Take a snapshot\n"
                "old = "
             << oldValue.str() << ";"
             << ";\n"
                "// Quit if snapshot meets condition.\n"
                "if( old<=recordValue ) break;\n"
                "// Attempt to install new value.\n"
                "} while( "
             << oldValue.str() << ".compare_and_swap(recordValue,old)!=old);";

  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_AGGREGATE, statements.str()));
  if (parent != nullptr) {
    parent->consume(cg);
  }
}

/*
 * Max
 */
void Max::produce(CodeGenerator &cg, Operator *input) {
  // add field to schema
  Schema schema = Schema::create().addFixSizeField(fieldId + "_max", DataType::Long, Stream);
  produce_(cg, input, schema);
  createState(cg, input, schema);
  migrateFrom(cg, input, schema);
  migrateTo(cg, input, schema);
  addStatePtr(cg, input, schema);
}

void Max::consume(CodeGenerator &cg, Operator *parent) {

  // calculate max
  std::stringstream statements;
  consume_(cg, parent);

  statements << "auto recordValue = record." << fieldId << "; ";

  std::stringstream oldValue;
  if (cg.config.getNuma()) {
    oldValue << "state" << std::to_string(pipeline) << "[bufferIndex][key]." << fieldId << "_max";
  } else {
    if (cg.ctx(pipeline).hasGroupBy) {
      oldValue << "state" << std::to_string(pipeline) << "[bufferIndex][key]." << fieldId << "_max";
    } else {
      oldValue << "state" << std::to_string(pipeline) << "[bufferIndex]." << fieldId << "_max";
    }
  }
  statements << "long old; do {\n"
                "// Take a snapshot\n"
                "old = "
             << oldValue.str() << ";"
             << ";\n"
                "// Quit if snapshot meets condition.\n"
                "if( old>=recordValue ) break;\n"
                "// Attempt to install new value.\n"
                "} while( "
             << oldValue.str() << ".compare_and_swap(recordValue,old)!=old);";
  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_AGGREGATE, statements.str()));

  if (parent != nullptr) {
    parent->consume(cg);
  }
}

bool Avg::hasFinalAggregation() { return true; }

/*
 * Avg
 */
void Avg::produce(CodeGenerator &cg, Operator *input) {
  // add three fields to schema (save count and sum to calculate avg later)
  Schema schema = Schema::create()
                      .addFixSizeField(fieldId + "_avg", DataType::Double, Stream)
                      .addFixSizeField(fieldId + "_sum", DataType::Long, Stream)
                      .addFixSizeField("count", DataType::Long, Stream);

  produce_(cg, input, schema);
  createState(cg, input, schema);
  migrateFrom(cg, input, schema);
  migrateTo(cg, input, schema);
  addStatePtr(cg, input, schema);
}

void Avg::consume(CodeGenerator &cg, Operator *parent) {

  consume_(cg, parent);
  std::stringstream statements;
  statements << "auto recordValue = record." << fieldId << "; ";

  std::stringstream oldValue;
  if (cg.config.getNuma()) {
    oldValue << "state" << std::to_string(pipeline) << "[bufferIndex][key]";
  } else {
    if (cg.ctx(pipeline).hasGroupBy) {
      oldValue << "state" << std::to_string(pipeline) << "[bufferIndex][key]";
    } else {
      oldValue << "state" << std::to_string(pipeline) << "[bufferIndex]";
    }
  }

  statements << oldValue.str() << ".count++;" << std::endl;
  statements << oldValue.str() << "." << fieldId << "_sum+= recordValue;" << std::endl;
  cg.pipeline(pipeline).addInstruction(CMethod::Instruction(INSTRUCTION_AGGREGATE, statements.str()));
  if (parent != nullptr) {
    parent->consume(cg);
  }
}

void Avg::consumeFinalAggregation(CodeGenerator &cg, Operator *pOperator) {
  std::stringstream statements;
  statements << "if(record.count != 0)";
  statements << "record." << fieldId << "_avg"
             << " = "
             << "((double)record." << fieldId << "_sum) / ((double)record.count);";
  cg.pipeline(pipeline - 1).addInstruction(CMethod::Instruction(INSTRUCTION_AGGREGATE, statements.str()));
}

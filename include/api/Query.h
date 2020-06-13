#ifndef API_QUERY_H
#define API_QUERY_H

#include <cassert>
#include <string>

#include "api/Aggregation.h"
#include "api/Config.h"
#include "api/Mapper.h"
#include "api/Predicate.h"
#include "api/Schema.h"
#include "api/Window.h"
#include "operator/Operator.h"

class Query {
public:
  ~Query();
  static Query generate(Config &config, Schema &schema, std::string path);
  void generate();
  void execute();

  // relational operators
  Query &filter(Predicate &&predicate) { return filter(predicate); };
  Query &filter(Predicate &predicate);
  Query &filter(Predicate *predicate) { return filter(*predicate); };
  Query &select(std::vector<std::string> fields);
  Query &groupBy(std::string fieldId);
  Query &groupBy(std::string fieldId, int keyRange);
  Query &aggregate(Aggregation &&aggregation);

  // streaming operators
  Query &window(Window &&window);
  Query &map(Mapper &&mapper);

  // input operators
  Query &input(InputType type, std::string path);

  // output operators
  Query &write(std::string fileName);
  Query &print();

  // helper operators
  static void printQueryPlan(Query query);
  static void printPipelinePermutations(Query query);

  Operator *root;
  Operator *getInputOperator() { return root; }

  Query &toOutputBuffer();
  Schema &schema;
  Config &config;
  Operator *current;

private:
  Query(Config &config, Schema &schema);

  static void printQueryPlan(Operator *curr, int depth);
};

#endif // API_QUERY_H

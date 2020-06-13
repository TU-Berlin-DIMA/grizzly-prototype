#ifndef OPERATOR_MAP_OPERATOR_H
#define OPERATOR_MAP_OPERATOR_H

#include "api/Aggregation.h"
#include "api/Mapper.h"
#include "operator/Operator.h"

class MapOperator : public Operator {
public:
  MapOperator(Mapper &mapper, Operator *input);
  ~MapOperator();
  void consume(CodeGenerator &cg);
  void produce(CodeGenerator &cg);
  std::string to_string();

private:
  Mapper &mapper;
  Operator *input;
};

#endif // OPERATOR_MAP_OPERATOR_H

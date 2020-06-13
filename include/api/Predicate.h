#ifndef API_PREDICATE_H
#define API_PREDICATE_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "api/Schema.h"
#include "code_generation/CodeGenerator.h"
#include "operator/Operator.h"

class Predicate {
public:
  Predicate(std::string fieldId, std::string value) : fieldId(fieldId), value("\"" + value + "\""){};

  Predicate(std::string fieldId, long value) : fieldId(fieldId), value(std::to_string(value)){};

  Predicate(std::string fieldId, Field &field2) : fieldId(fieldId), value("record." + field2.name){};

  virtual ~Predicate() {}

  std::string fieldId;
  std::string value;
  Field *field;
  std::vector<Predicate *> ands;
  std::vector<Predicate *> ors;

  virtual std::string to_string() { return "predicate"; };

  virtual void produce(CodeGenerator &cg, Operator *parent);

  virtual void consume(CodeGenerator &cg, Operator *parent);

  Predicate *And(Predicate *And) {
    ands.push_back(And);
    return this;
  }

  virtual Predicate &And(Predicate &&And) {
    ands.push_back(&And);
    return *this;
  }

  Predicate *Or(Predicate *Or) {
    ors.push_back(Or);
    return this;
  }

  virtual Predicate &Or(Predicate &&Or) {
    ors.push_back(&Or);
    return *this;
  }

protected:
  size_t pipeline;

  virtual std::string generate(CodeGenerator &c) = 0;

  virtual std::string generatProfilingCode(CodeGenerator &c) {
    c.profilingDataManager->registerSelectivityHandler("select", 1 + ands.size() + ors.size());
    std::stringstream buffer;
    buffer << "if(thread_id==0){\n";
    buffer << "auto sel = variant->profilingDataManager->getSelectivityHandler(\"select\");\n";
    int i = 0;
    buffer << "sel->update(" << i++ << "," << generate(c) << ");\n";

    for (Predicate *a : ands) {
      buffer << "sel->update(" << i++ << "," << a->generateAll(c) << ");\n";
    }

    buffer << "sel->operator++();}\n";

    return buffer.str();
  }

  virtual std::string generateAllOptimized(CodeGenerator &c) {
    SelectivityHandler *handler = c.profilingDataManager->getSelectivityHandler("select");
    auto values = handler->getValue();

    std::vector<uint64_t> v;
    std::vector<std::string> predicates;
    predicates.push_back(generate(c));
    for (Predicate *a : ands) {
      predicates.push_back(a->generateAll(c));
    }

    std::cout << "Counted Selectivity across " << handler->counter << " Tuple" << std::endl;

    for (uint64_t i = 0; i < (1 + ands.size() + ors.size()); i++) {
      v.push_back(values[i]);
      std::cout << "Selectivity of predicate: " << i << " is " << ((double)values[i] / (double)handler->counter)
                << std::endl;
    }

    int s = v.size();
    std::string predicate;
    for (int i = 0; i < s; i++) {
      uint64_t minI = 0;
      for (int b = 1; b < s; b++) {
        if (v[minI] > v[b]) {
          minI = b;
        }
      }
      v[minI] = INT64_MAX;
      if (i == 0)
        predicate = predicates[minI];
      else
        predicate += " && (" + predicates[minI] + ")";
    }

    /*

    std::string predicate = generate(c);

    // add AND-predicates
    if (ands.size() > 0) {
        predicate += " && (";
        int i = 0;
        for (Predicate *a : ands) {
            predicate += a->generateAll(c);
            i++;
            if (i < ands.size()) {
                predicate += " && ";
            }
        }
        predicate += ")";
    }

    // add OR-predicates
    if (ors.size() > 0) {
        predicate += " || (";
        for (Predicate *o : ors) {
            predicate += o->generateAll(c);
        }
        predicate += ")";
    }
     */
    return predicate;
  }

  virtual std::string generateAll(CodeGenerator &c) {

    std::string predicate = generate(c);

    // add AND-predicates
    if (ands.size() > 0) {
      predicate += " && (";
      int i = 0;
      for (Predicate *a : ands) {
        predicate += a->generateAll(c);
        i++;
        if (i < ands.size()) {
          predicate += " && ";
        }
      }
      predicate += ")";
    }

    // add OR-predicates
    if (ors.size() > 0) {
      predicate += " || (";
      for (Predicate *o : ors) {
        predicate += o->generateAll(c);
      }
      predicate += ")";
    }
    return predicate;
  }
};

class Equal : public Predicate {
public:
  Equal(std::string fieldId, std::string value) : Predicate(fieldId, value) {}

  Equal(std::string fieldId, long value) : Predicate(fieldId, value) {}

  Equal(std::string fieldId, Field &field2) : Predicate(fieldId, field2){};

  std::string to_string() override;

protected:
  std::string generate(CodeGenerator &c) override;
};

class NotEqual : public Predicate {
public:
  NotEqual(std::string fieldId, std::string value) : Predicate(fieldId, value) {}

  NotEqual(std::string fieldId, long value) : Predicate(fieldId, value) {}

  NotEqual(std::string fieldId, Field &field2) : Predicate(fieldId, field2){};

  std::string to_string() override;

protected:
  std::string generate(CodeGenerator &c) override;
};

class Greater : public Predicate {
public:
  Greater(std::string fieldId, std::string value) : Predicate(fieldId, value) {}

  Greater(std::string fieldId, long value) : Predicate(fieldId, value) {}

  Greater(std::string fieldId, Field &field2) : Predicate(fieldId, field2){};

  std::string to_string() override;

protected:
  std::string generate(CodeGenerator &c) override;
};

class GreaterEqual : public Predicate {
public:
  GreaterEqual(std::string fieldId, std::string value) : Predicate(fieldId, value) {}

  GreaterEqual(std::string fieldId, long value) : Predicate(fieldId, value) {}

  GreaterEqual(std::string fieldId, Field &field2) : Predicate(fieldId, field2){};

  std::string to_string() override;

protected:
  std::string generate(CodeGenerator &c) override;
};

class Less : public Predicate {
public:
  Less(std::string fieldId, std::string value) : Predicate(fieldId, value) {}

  Less(std::string fieldId, long value) : Predicate(fieldId, value) {}

  Less(std::string fieldId, Field &field2) : Predicate(fieldId, field2){};

  std::string to_string() override;

protected:
  std::string generate(CodeGenerator &c) override;
};

class LessEqual : public Predicate {
public:
  LessEqual(std::string fieldId, std::string value) : Predicate(fieldId, value) {}

  LessEqual(std::string fieldId, long value) : Predicate(fieldId, value) {}

  LessEqual(std::string fieldId, Field &field2) : Predicate(fieldId, field2){};

  std::string to_string() override;

protected:
  std::string generate(CodeGenerator &c) override;
};

class Like : public Predicate {
public:
  Like(std::string fieldId, std::string value) : Predicate(fieldId, value) {}

  Like(std::string fieldId, long value) : Predicate(fieldId, value) {}

  Like(std::string fieldId, Field &field2) : Predicate(fieldId, field2){};

  std::string to_string() override;

protected:
  std::string generate(CodeGenerator &c) override;
};

#endif // API_PREDICATE_H

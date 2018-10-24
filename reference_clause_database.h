#pragma once

#include "clause_database.h"

struct eager_restricted_clause : clause_pointer {
  std::vector<literal> literals;
  eager_restricted_clause(const proof_clause& c) :
  clause_pointer({&c,0}), literals(c.begin(), c.end()) {}
  bool unit() const { return not satisfied and literals.size() == 1; }
  void assert_unit(const std::vector<int>& assignment) const {
    assert(unit());
  }
  bool contradiction() const { return not satisfied and literals.empty(); }
  branch propagate() const { return {literals.front(), source}; }

  void restrict(literal l);
  void loosen(literal l);
  void restrict(const std::vector<int>& assignment);
  void restrict_to_unit(const std::vector<int>& assignment) {
    restrict(assignment);
  }
  void reset();
};
std::ostream& operator << (std::ostream& o, const eager_restricted_clause& c);

struct lazy_restricted_clause : clause_pointer {
  literal satisfied_literal;
  int unassigned;
  boost::dynamic_bitset<> literals;
  lazy_restricted_clause(const proof_clause& c) :
  clause_pointer({&c,0}), satisfied_literal(0,false),
    unassigned(source->c.width()), literals(unassigned) {
      assert(std::is_sorted(source->begin(), source->end()));
      literals.set();
    }
  bool unit() const { return not satisfied and unassigned == 1; }
  void assert_unit(const std::vector<int>& assignment) const {
    assert(unit());
  }
  bool contradiction() const { return not satisfied and unassigned == 0; }
  branch propagate() const;

  void restrict(literal l);
  void loosen(literal l);
  void restrict_to_unit(const std::vector<int>& assignment);
  void reset();
};
std::ostream& operator << (std::ostream& o, const lazy_restricted_clause& c);

template<typename T>
class reference_clause_database : public clause_database<T> {
public:
  reference_clause_database(std::vector<const proof_clause*>& conflicts,
                            struct propagation_queue& propagation_queue,
                            std::vector<int>& assignment,
                            std::vector<int>& decision_level) :
  clause_database<T>(conflicts, propagation_queue, assignment, decision_level) {}
  virtual ~reference_clause_database() {}
  virtual void assign(literal l);
  virtual void unassign(literal l);
  virtual void reset();
};

typedef reference_clause_database<eager_restricted_clause> eager_clause_database;
typedef reference_clause_database<lazy_restricted_clause> lazy_clause_database;

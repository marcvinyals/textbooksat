#pragma once

#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "data_structures.h"

struct eager_restricted_clause {
  std::vector<literal> literals;
  const proof_clause* source;
  int satisfied;
  eager_restricted_clause(const proof_clause& c) :
    literals(c.begin(), c.end()), source(&c), satisfied(0) {}
  bool unit() const { return not satisfied and literals.size() == 1; }
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

struct lazy_restricted_clause {
  const proof_clause* source;
  bool satisfied;
  literal satisfied_literal;
  int unassigned;
  boost::dynamic_bitset<> literals;
  lazy_restricted_clause(const proof_clause& c) :
    source(&c), satisfied(false), satisfied_literal(0,false),
    unassigned(source->c.width()), literals(unassigned) {
      assert(std::is_sorted(source->begin(), source->end()));
      literals.set();
    }
  bool unit() const { return not satisfied and unassigned == 1; }
  bool contradiction() const { return not satisfied and unassigned == 0; }
  branch propagate() const;

  void restrict(literal l);
  void loosen(literal l);
  void restrict_to_unit(const std::vector<int>& assignment);
  void reset();
};
std::ostream& operator << (std::ostream& o, const lazy_restricted_clause& c);

typedef lazy_restricted_clause restricted_clause;

template<typename T>
class clause_database {
protected:
  std::vector<T> working_clauses;
  std::vector<const proof_clause*>& conflicts;
  struct propagation_queue& propagation_queue;
public:
  clause_database(std::vector<const proof_clause*>& conflicts,
                  struct propagation_queue& propagation_queue) :
    conflicts(conflicts),
    propagation_queue(propagation_queue) {}
  
  virtual void assign(literal l)=0;
  virtual void unassign(literal l)=0;
  virtual void reset()=0;

  virtual void set_variables(size_t variables) {}
  virtual void insert(const proof_clause& c) { working_clauses.push_back(c); }
  virtual void insert(const proof_clause& c, const std::vector<int>& assignment) {
    insert(c);
    working_clauses.back().restrict_to_unit(assignment);
    assert(working_clauses.back().unit());
  }
  auto begin() const { return working_clauses.begin(); }
  auto end() const { return working_clauses.end(); }
  auto begin() { return working_clauses.begin(); }
  auto end() { return working_clauses.end(); }
  auto& back() { return working_clauses.back(); }
  size_t size() const { return working_clauses.size(); }
  auto erase(typename std::vector<T>::iterator it) {
    return working_clauses.erase(it);
  }
};

template<typename T>
class reference_clause_database : public clause_database<T> {
public:
  reference_clause_database(std::vector<const proof_clause*>& conflicts,
                            struct propagation_queue& propagation_queue) :
    clause_database<T>(conflicts, propagation_queue) {}
  virtual void assign(literal l);
  virtual void unassign(literal l);
  virtual void reset();
};

typedef reference_clause_database<eager_restricted_clause> eager_clause_database;
typedef reference_clause_database<lazy_restricted_clause> lazy_clause_database;


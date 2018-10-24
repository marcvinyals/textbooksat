#pragma once

#include <vector>

#include "data_structures.h"

struct clause_pointer {
  const proof_clause* source;
};

template<typename T>
class clause_database {
protected:
  std::vector<T> working_clauses;
  std::vector<const proof_clause*>& conflicts;
  struct propagation_queue& propagation_queue;
  const std::vector<int>& assignment;
  const std::vector<int>& decision_level;
public:
  clause_database(std::vector<const proof_clause*>& conflicts,
                  struct propagation_queue& propagation_queue,
		  const std::vector<int>& assignment,
		  const std::vector<int>& decision_level) :
    conflicts(conflicts),
    propagation_queue(propagation_queue),
    assignment(assignment),
    decision_level(decision_level) {}
  virtual ~clause_database() {}
  
  virtual void assign(literal l)=0;
  virtual void unassign(literal l)=0;
  virtual void reset()=0;
  virtual void fill_propagation_queue();

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

#pragma once

#include <vector>

#include "cast_iterator.h"
#include "data_structures.h"

struct clause_pointer {
  const proof_clause* source;
};

struct clause_database_i {
  virtual void set_variables(size_t variables) {}
  virtual ~clause_database_i() {}

  virtual void assign(literal l)=0;
  virtual void unassign(literal l)=0;
  virtual void reset()=0;
  virtual void fill_propagation_queue()=0;

  virtual void insert(const proof_clause& c)=0;
  virtual void insert(const proof_clause& c, const std::vector<int>& assignment)=0;

  typedef cast_iterator<clause_pointer> clause_iterator;

  virtual clause_iterator begin() const =0;
  virtual clause_iterator end() const =0;
  virtual clause_iterator erase(clause_iterator)=0;
  const clause_pointer& back() const { return *(end()-1); }
  size_t size() const { return end()-begin(); }

  virtual bool consistent() const =0;
};


template<typename T>
class clause_database : public clause_database_i {
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

  virtual void insert(const proof_clause& c) { working_clauses.push_back(c); }
  virtual void insert(const proof_clause& c, const std::vector<int>& assignment) {
    insert(c);
    working_clauses.back().restrict_to_unit(assignment);
    assert(working_clauses.back().unit());
  }
  virtual void fill_propagation_queue();

  virtual clause_iterator begin() const { return working_clauses.begin(); }
  virtual clause_iterator end() const { return working_clauses.end(); }

  virtual clause_iterator erase(clause_iterator it) {
    std::swap(working_clauses.back(),it.dereference_as<T>());
    working_clauses.pop_back();
    return it;
  }
  virtual bool consistent() const {
    for (auto& c : working_clauses) {
      assert(not c.contradiction());
    }
    return true;
  }
};

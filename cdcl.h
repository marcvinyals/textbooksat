#pragma once

#include <algorithm>
#include <vector>
#include <set>
#include <deque>
#include <list>
#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_set>

#include <boost/dynamic_bitset.hpp>

#include "data_structures.h"

struct literal_or_restart {
  literal l;
  bool restart;
  literal_or_restart(literal l_) : l(l_), restart(false) {}
  literal_or_restart(bool restart_) : l(0,0), restart(restart_) {}
};

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

struct lazy_restricted_clause {
  const proof_clause* source;
  bool satisfied;
  int unassigned;
  boost::dynamic_bitset<> literals;
  lazy_restricted_clause(const proof_clause& c) :
  source(&c), satisfied(false), unassigned(source->c.width()), literals(unassigned) {
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

typedef eager_restricted_clause restricted_clause;

struct propagation_queue {
  std::deque<branch> q;
  void propagate(const restricted_clause& c) {
    assert(c.unit());
    q.push_back(c.propagate());
  }
  void decide(literal l) {
    q.push_back({l,NULL});
  }
  void clear() {
    q.clear();
  }
  void pop() {
    q.pop_front();
  }
  bool empty() const {
    return q.empty();
  }
  const branch& front() const {
    return q.front();
  }
};

class cdcl {
 public:
  cdcl() {}
  
  proof solve(const cnf& f);

  std::function<literal_or_restart(cdcl&)> decide_plugin;
  std::function<bool(cdcl&)> restart_plugin;
std::function<proof_clause(cdcl&, const branching_sequence::reverse_iterator&)> learn_plugin;
  std::function<bool(const cdcl&, int, int)> variable_order_plugin;
  std::function<std::unordered_set<variable>(const proof_clause&)> bump_plugin;
  std::function<void(cdcl&)> forget_plugin;
#ifndef NO_VIZ
  std::function<void(const std::vector<int>&, const std::vector<restricted_clause>&)> visualizer_plugin;
#endif

  bool config_backjump;
  bool config_minimize;
  bool config_phase_saving;
  double config_activity_decay;

  literal_or_restart decide_fixed();
  literal_or_restart decide_random();
  literal_or_restart decide_ask();

  bool restart_none();
  bool restart_fixed();

  proof_clause learn_fuip(const branching_sequence::reverse_iterator& first_decision);
  proof_clause learn_fuip_all(const branching_sequence::reverse_iterator& first_decision);
  proof_clause learn_luip(const branching_sequence::reverse_iterator& first_decision);
  proof_clause learn_decision(const branching_sequence::reverse_iterator& first_decision);

  bool variable_cmp_vsids(variable, variable) const;
  bool variable_cmp_fixed(variable, variable) const;
  bool variable_cmp_reverse(variable, variable) const;

  static std::unordered_set<variable> bump_learnt(const proof_clause& c);
  static std::unordered_set<variable> bump_conflict(const proof_clause& c);

  void forget_nothing();
  void forget_everything();
  void forget_wide();
  void forget_wide(unsigned int w);
  void forget_domain(const std::vector<variable>& domain);
  void forget_touches_all(const std::vector<variable>& domain);
  void forget_touches_any(const std::vector<variable>& domain);
  void forget_if(const std::function<bool(clause)>& predicate);

  std::shared_ptr<std::ostream> trace;
  
private:
  friend class ui;

  void unit_propagate();
  void learn();
  void forget(unsigned int m);
  void decide();
  void restart();
  
  void assign(literal l);
  void unassign(literal l);
  bool asserting(const proof_clause& c) const;
  void backjump(const proof_clause& learnt_clause,
                const branching_sequence::reverse_iterator& first_decision,
                branching_sequence::reverse_iterator& backtrack_limit);
  void minimize(proof_clause& c) const;
  void bump_activity(const proof_clause& c);

  bool consistent() const;
  
  bool solved; // Done
  std::vector<const proof_clause*> conflicts;
  
  // Copy of the formula.
  std::vector<proof_clause> formula;
  // Learnt clauses, in order. We will pointers to proof clauses, so
  // they should not be erased or reallocated.
  std::list<proof_clause> learnt_clauses;
  // Clauses restricted to the current assignment.
  std::vector<restricted_clause> working_clauses;

  // List of unit propagations, in chronological order.
  branching_sequence branching_seq;
  // Reasons for propagation, indexed by literal number. If a
  // propagated literal does not have any reason, then it was
  // decided. It is possible for a literal to have multiple reasons
  // before being propagated; we choose the first as the main reason.
  std::vector<std::list<const proof_clause*>> reasons;
  // Queue of literals waiting to be unit-propagated.
  struct propagation_queue propagation_queue;
  // Assignment induced by the branching sequence, indexed by variable
  // number. Possible values are 1 (true), -1 (false) or 0
  // (unassigned).
  std::vector<int> assignment;

  // Queue of variables waiting to be decided.
  typedef std::function<bool(variable, variable)> variable_cmp;
  std::set<variable, variable_cmp> decision_order;
  // Value a decided variable should be set to, indexed by variable
  // number.
  std::vector<bool> decision_polarity;
  std::vector<double> variable_activity;
};

std::ostream& operator << (std::ostream& o, const eager_restricted_clause& c);
std::ostream& operator << (std::ostream& o, const lazy_restricted_clause& c);
std::ostream& operator << (std::ostream& o, const std::list<proof_clause>& v);
std::ostream& operator << (std::ostream& o, const branch& b);

template<typename T>
std::ostream& operator << (std::ostream& o, const std::vector<T>& v) {
  for (const auto& i:v) o << i;
  return o;
}
std::ostream& operator << (std::ostream& o, const std::vector<clause>& v);

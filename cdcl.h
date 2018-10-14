#pragma once

#include <algorithm>
#include <vector>
#include <set>
#include <list>
#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_set>

#include "clause_database.h"
#include "data_structures.h"
#include "propagation_queue.h"

struct literal_or_restart {
  literal l;
  bool restart;
  literal_or_restart(literal l_) : l(l_), restart(false) {}
  literal_or_restart(bool restart_) : l(0,0), restart(restart_) {}
};

class cdcl {
 public:
  cdcl() : working_clauses(conflicts, propagation_queue) {}
  
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
  std::vector<double> initial_variable_activity(const cnf& f);

  bool consistent() const;
  
  bool solved; // Done
  std::vector<const proof_clause*> conflicts;
  
  // Copy of the formula.
  std::vector<proof_clause> formula;
  // Learnt clauses, in order. We will pointers to proof clauses, so
  // they should not be erased or reallocated.
  std::list<proof_clause> learnt_clauses;
  // Clauses restricted to the current assignment.
  clause_database working_clauses;

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
  // Number of decisions appearing before or at a literal in the
  // branching sequence. -1 if the literal is not assigned.
  std::vector<int> decision_level;

  // Queue of variables waiting to be decided.
  typedef std::function<bool(variable, variable)> variable_cmp;
  std::set<variable, variable_cmp> decision_order;
  // Value a decided variable should be set to, indexed by variable
  // number.
  std::vector<bool> decision_polarity;
  std::vector<double> variable_activity;
};

std::ostream& operator << (std::ostream& o, const std::list<proof_clause>& v);
std::ostream& operator << (std::ostream& o, const branch& b);

template<typename T>
std::ostream& operator << (std::ostream& o, const std::vector<T>& v) {
  for (const auto& i:v) o << i;
  return o;
}
std::ostream& operator << (std::ostream& o, const std::vector<clause>& v);

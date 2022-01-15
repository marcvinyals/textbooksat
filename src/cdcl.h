#pragma once

#include <algorithm>
#include <vector>
#include <set>
#include <list>
#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_set>

#include "reference_clause_database.h"
#include "watched_clause_database.h"
#include "data_structures.h"
#include "propagation_queue.h"

class cdcl {
 public:
  template<typename T>
    cdcl(T*) : working_clauses(*(new T(conflicts, propagation_queue, assignment, decision_level))) {}
  cdcl(const cdcl&) = delete;
  cdcl(cdcl&&) = default;
  cdcl& operator = (const cdcl&) = delete;
  ~cdcl() { delete &working_clauses; }

  proof solve(const cnf& f);

  std::function<literal(cdcl&)> decide_plugin;
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
  bool config_default_polarity;
  double config_activity_decay;
  double config_clause_decay;

  literal decide_fixed();
  literal decide_random();

  bool restart_none();
  bool restart_always();
  bool restart_luby();

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
  void bump_clause_activity(const proof_clause& c);
  std::vector<double> initial_variable_activity(const cnf& f);

  bool consistent() const;
  bool stable() const;
  void display_stats() const;
  
  bool solved; // Done
  std::vector<const proof_clause*> conflicts;
  
  // Copy of the formula.
  std::vector<proof_clause> formula;
  // Learnt clauses, in order. We will pointers to proof clauses, so
  // they should not be erased or reallocated.
  std::list<proof_clause> learnt_clauses;
  // Clauses restricted to the current assignment.
  clause_database_i& working_clauses;

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
  // Exponential moving average of each variable appearing in a conflict.
  std::vector<double> variable_activity;
  double variable_activity_bump = 1;
  // Exponential moving average of each clause appearing in a conflict.
  std::unordered_map<const proof_clause*, double> clause_activity;
  double clause_activity_bump = 1;

  struct stats {
    int decisions, propagations, conflicts, restarts;
    stats() : decisions(0), propagations(0), conflicts(0), restarts(0) {}
  } stats;
};

std::ostream& operator << (std::ostream& o, const std::list<proof_clause>& v);
std::ostream& operator << (std::ostream& o, const branch& b);

template<typename T>
std::ostream& operator << (std::ostream& o, const std::vector<T>& v) {
  for (const auto& i:v) o << i;
  return o;
}
std::ostream& operator << (std::ostream& o, const std::vector<clause>& v);

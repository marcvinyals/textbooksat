#pragma once

#include "clause_database.h"

#include "reference_clause_database.h"

struct watched_clause : clause_pointer {
  std::vector<literal> literals;
  size_t literals_visible_size;
  int unassigned;
  int satisfied;

  watched_clause(const proof_clause& c) :
    clause_pointer({&c}), literals(c.begin(), c.end()), literals_visible_size(literals.size()),
    unassigned(std::min(2,int(literals_visible_size))), satisfied(0) {}

  bool unit() const { return not satisfied and unassigned == 1; }
  void assert_unit(const std::vector<int>& assignment) const {
#ifdef DEBUG
    assert(unit());
    eager_restricted_clause c(*source);
    c.restrict(assignment);
    assert(c.unit());
#endif
  }
  bool contradiction() const { return not satisfied and unassigned == 0; }
  branch propagate() const { return {literals[0], source}; }

  literal restrict(literal l, const std::vector<int>& assignment);
  void satisfy();
  void loosen_satisfied(literal l);
  void loosen_falsified(literal l);
  void restrict_to_unit(const std::vector<int>& assignment) { assert(false); }
  void restrict_to_unit(const std::vector<int>& assignment,
                        const std::vector<int>& decision_level);
  void reset();
private:
  literal find_new_watch(size_t replaces, const std::vector<int>& assignment);
};
std::ostream& operator << (std::ostream& o, const watched_clause& c);

class watched_clause_database : public clause_database<watched_clause> {
private:
  std::vector<std::vector<size_t>> watches;
public:
  watched_clause_database(std::vector<const proof_clause*>& conflicts,
                          struct propagation_queue& propagation_queue,
                          const std::vector<int>& assignment,
                          const std::vector<int>& decision_level) :
    clause_database(conflicts, propagation_queue, assignment, decision_level) {}
  virtual ~watched_clause_database() {}
  virtual void assign(literal l);
  virtual void unassign(literal l);
  virtual void reset();

  virtual void set_variables(size_t variables) { watches.resize(2*variables); }
  virtual void insert(const proof_clause& c);
  virtual void insert(const proof_clause& c, const std::vector<int>& assignment);
};

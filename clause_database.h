#pragma once

#include <vector>

#include <boost/dynamic_bitset.hpp>
#include <boost/iterator/indirect_iterator.hpp>

#include "data_structures.h"

struct clause_pointer {
  const proof_clause* source;
};
std::ostream& operator << (std::ostream& o, const clause_pointer& c);

struct eager_restricted_clause : clause_pointer {
  std::vector<literal> literals;
  int satisfied;
  eager_restricted_clause(const proof_clause& c) :
  clause_pointer({&c}), literals(c.begin(), c.end()), satisfied(0) {}
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
  bool satisfied;
  literal satisfied_literal;
  int unassigned;
  boost::dynamic_bitset<> literals;
  lazy_restricted_clause(const proof_clause& c) :
  clause_pointer({&c}), satisfied(false), satisfied_literal(0,false),
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

class clause_database {
protected:
  std::vector<std::unique_ptr<clause_pointer>> working_clauses;
  std::vector<const proof_clause*>& conflicts;
  struct propagation_queue& propagation_queue;
public:
  clause_database(std::vector<const proof_clause*>& conflicts,
                  struct propagation_queue& propagation_queue) :
    conflicts(conflicts),
    propagation_queue(propagation_queue) {}
  virtual ~clause_database() {}
  
  virtual void assign(literal l)=0;
  virtual void unassign(literal l)=0;
  virtual void reset()=0;
  virtual void fill_propagation_queue()=0;

  virtual void set_variables(size_t variables) {}
  virtual void insert(const proof_clause& c)=0;
  virtual void insert(const proof_clause& c, const std::vector<int>& assignment)=0;
  typedef boost::indirect_iterator<decltype(working_clauses.begin())> it_t;
  typedef boost::indirect_iterator<decltype(working_clauses.cbegin())> cit_t;
  cit_t begin() const { return working_clauses.cbegin(); }
  cit_t end() const { return working_clauses.cend(); }
  it_t begin() { return working_clauses.begin(); }
  it_t end() { return working_clauses.end(); }
  auto& back() { return *working_clauses.back(); }
  size_t size() const { return working_clauses.size(); }
  auto erase(it_t it) {
    return working_clauses.erase(it.base());
  }
};

template<typename T>
class reference_clause_database : public clause_database {
public:
  reference_clause_database(std::vector<const proof_clause*>& conflicts,
                            struct propagation_queue& propagation_queue,
                            std::vector<int>& assignment,
                            std::vector<int>& decision_level) :
    clause_database(conflicts, propagation_queue) {}
  virtual ~reference_clause_database() {}
  virtual void assign(literal l);
  virtual void unassign(literal l);
  virtual void insert(const proof_clause& c) {
    working_clauses.push_back(std::make_unique<T>(c));
  }
  virtual void insert(const proof_clause& c, const std::vector<int>& assignment) {
    insert(c);
    ((T*)working_clauses.back().get())->restrict_to_unit(assignment);
    assert(((T*)working_clauses.back().get())->unit());
  }
  virtual void reset();
  virtual void fill_propagation_queue();

  typedef boost::indirect_iterator<decltype(working_clauses.begin()),T> it_t;
  typedef boost::indirect_iterator<decltype(working_clauses.cbegin()),T> cit_t;
  cit_t begin() const { return working_clauses.cbegin(); }
  cit_t end() const { return working_clauses.cend(); }
  it_t begin() { return working_clauses.begin(); }
  it_t end() { return working_clauses.end(); }
};

typedef reference_clause_database<eager_restricted_clause> eager_clause_database;
typedef reference_clause_database<lazy_restricted_clause> lazy_clause_database;










struct watched_clause : clause_pointer {
  std::vector<literal> literals;
  size_t literals_visible_size;
  int unassigned;
  int satisfied;

  watched_clause(const proof_clause& c) :
  clause_pointer({&c}),
    literals(c.begin(), c.end()), literals_visible_size(literals.size()),
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

struct source_hash {
  size_t operator () (const watched_clause* key) const {
    return std::hash<const proof_clause*>()(key->source);
  }
};
struct source_cmp {
  bool operator () (const watched_clause* c1, const watched_clause* c2) const {
    return c1->source==c2->source;
  }
};

class watched_clause_database : public clause_database {
private:
  std::vector<std::vector<watched_clause*>> watches;
  const std::vector<int>& assignment;
  const std::vector<int>& decision_level;
public:
  watched_clause_database(std::vector<const proof_clause*>& conflicts,
                          struct propagation_queue& propagation_queue,
                          const std::vector<int>& assignment,
                          const std::vector<int>& decision_level) :
    clause_database(conflicts, propagation_queue),
    assignment(assignment),
    decision_level(decision_level) {}
  virtual ~watched_clause_database() {}
  virtual void assign(literal l);
  virtual void unassign(literal l);
  virtual void reset();
  virtual void fill_propagation_queue();

  virtual void set_variables(size_t variables) { watches.resize(2*variables); }
  virtual void insert(const proof_clause& c);
  virtual void insert(const proof_clause& c, const std::vector<int>& assignment);
};

typedef watched_clause restricted_clause;

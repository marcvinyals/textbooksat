#pragma once

#include <vector>

#include <boost/dynamic_bitset.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include "data_structures.h"

struct clause_pointer {
  const proof_clause* source;
  int satisfied;
};
std::ostream& operator << (std::ostream& o, const clause_pointer& c);

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

struct clause_database_i {
  virtual ~clause_database_i() {}
  virtual void assign(literal l)=0;
  virtual void unassign(literal l)=0;
  virtual void reset()=0;
  virtual void fill_propagation_queue()=0;

  virtual void insert(const proof_clause& c)=0;
  virtual void insert(const proof_clause& c, const std::vector<int>& assignment)=0;

  struct it_t : public boost::iterator_facade<it_t,clause_pointer,std::random_access_iterator_tag> {
    const size_t stride;
    char* pointer;
    template<typename T>
      it_t(T it) : stride(sizeof(typename std::iterator_traits<T>::value_type)), pointer(const_cast<char*>(reinterpret_cast<const char*>((&(*it))))) {}
    reference dereference() const { return *reinterpret_cast<clause_pointer*>(pointer); }
    bool equal(const it_t& other) const { return pointer==other.pointer; }
    void increment() { pointer+=stride; }
    void decrement() { pointer-=stride; }
    void advance(ptrdiff_t n) { pointer+=(n*stride); }
    ptrdiff_t distance_to(const it_t& other) const { return (other.pointer-pointer)/stride; }
  };

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

  virtual void set_variables(size_t variables) {}
  virtual void insert(const proof_clause& c) { working_clauses.push_back(c); }
  virtual void insert(const proof_clause& c, const std::vector<int>& assignment) {
    insert(c);
    working_clauses.back().restrict_to_unit(assignment);
    assert(working_clauses.back().unit());
  }
  virtual void fill_propagation_queue();

  it_t begin() const { return it_t(working_clauses.begin()); }
  it_t end() const { return working_clauses.end(); }
  auto& back() { return working_clauses.back(); }
  size_t size() const { return working_clauses.size(); }
  /*auto erase(it_t it) {
    return working_clauses.erase(it); // FIXME
  }*/
};

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










struct watched_clause : clause_pointer {
  std::vector<literal> literals;
  size_t literals_visible_size;
  int unassigned;

  watched_clause(const proof_clause& c) :
  clause_pointer({&c,0}),
    literals(c.begin(), c.end()), literals_visible_size(literals.size()),
    unassigned(std::min(2,int(literals_visible_size))) {}

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

typedef watched_clause restricted_clause;

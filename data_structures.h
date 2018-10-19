#pragma once

#include <vector>
#include <string>
#include <list>
#include <unordered_map>

#include <boost/functional/hash.hpp>

typedef unsigned int variable;

struct literal {
  unsigned int l;
  literal(variable index, bool polarity) : l(index<<1) {
    if (polarity) l|=1;
  }
  static literal from_dimacs(int x) {
    if (x>0) return literal(x-1,true);
    else return literal(-x-1,false);
  }
  constexpr static literal from_raw(unsigned int x) { return x; }
  bool operator == (const literal& a) const { return l==a.l; }
  bool opposite(const literal& a) const { return (l^a.l) == 1; }
  literal abs() const { return l&(-2); }
  literal operator ~ () const { return l^1; }
  explicit operator variable() const { return l >> 1; }
  bool polarity() const { return l&1; }
  bool operator < (const literal& a) const { return l < a.l; }
protected:
  literal();
  constexpr literal(unsigned int l_) : l(l_) {}
};
namespace std {
  template<>
    struct hash<literal> {
    size_t operator () (literal key) const {
      return hash<int>()(key.l);
    }
  };
};
namespace boost {
  template<>
    struct hash<literal> {
    size_t operator () (literal key) const {
      return hash<int>()(key.l);
    }
  };
};

struct clause {
  // Sorted vector
  clause() {}
  clause(std::vector<literal> from_literals) : literals(from_literals) {}
  bool subsumes(const clause& c) const;
  bool subsumes(const clause& c, literal l) const;
  bool operator == (const clause& c) const { return literals == c.literals; }
  bool contains(literal l) const;
  typedef std::vector<literal>::const_iterator literal_iterator;
  literal_iterator begin() const { return literals.begin(); }
  literal_iterator end() const { return literals.end(); }
  std::size_t width() const { return literals.size(); }
  struct variable_iterator : public literal_iterator {
  variable_iterator(const literal_iterator&& it) : literal_iterator(it) {}
    variable operator * () const {
      return variable(literal_iterator::operator *());
    }
  };
  variable_iterator dom_begin() const { return literals.begin(); }
  variable_iterator dom_end() const { return literals.end(); }
private:
  std::vector<literal> literals;
  friend struct std::hash<clause>;
};
clause resolve(const clause& c, const clause& d, variable x);
clause resolve(const clause& c, const clause& d);
namespace std {
  template<>
    struct hash<clause> {
    size_t operator () (const clause& key) const {
      return boost::hash<vector<literal>>()(key.literals);
    }
  };
};

struct cnf {
  std::vector<clause> clauses;
  int variables;
  std::unordered_map<int,std::string> variable_names;
};

struct proof_clause;
struct branch {
  literal to;
  const proof_clause* reason;
};
typedef std::vector<branch> branching_sequence;

struct proof_clause {
  clause c;
  std::vector<const proof_clause*> derivation;
  branching_sequence trail;
  proof_clause(const clause& from_c) : c(from_c) {}
  proof_clause(const proof_clause&) = delete;
  proof_clause(proof_clause&&) = default;
  void resolve(const proof_clause& d, int x) {
    c = ::resolve(c, d.c, x);
    derivation.push_back(&d);
  }
  std::vector<literal>::const_iterator begin() const { return c.begin(); }
  std::vector<literal>::const_iterator end() const { return c.end(); }
};

struct proof {
  std::vector<proof_clause> formula;
  std::list<proof_clause> resolution;
  proof(const proof&) = delete;
  proof& operator = (const proof&) = delete;
  proof(proof&&) = default;
  proof(std::vector<proof_clause>&& from_formula,
        std::list<proof_clause>&& from_resolution) :
  formula(std::move(from_formula)), resolution(std::move(from_resolution)) {}
};

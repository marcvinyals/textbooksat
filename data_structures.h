#pragma once

#include <vector>
#include <string>
#include <list>
#include <map>

#include <boost/functional/hash.hpp>

typedef unsigned int variable;

struct literal {
  int l;
  literal(int variable, bool polarity) : l(variable<<1) {
    if (polarity) l|=1;
  }
  bool operator == (const literal& a) const { return l==a.l; }
  bool opposite(const literal& a) const { return (l^a.l) == 1; }
  literal operator ~ () const { return l^1; }
  explicit operator variable() const { return l >> 1; }
  bool polarity() const { return l&1; }
  bool operator < (const literal& a) const { return l < a.l; }
protected:
  literal();
  literal(int l_) : l(l_) {}
  friend literal from_dimacs(int);
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

inline literal from_dimacs(int x) {
  if (x>0) return literal(x-1,true);
  else return literal(-x-1,false);
}

struct clause {
  // Sorted vector
  clause() {}
  clause(std::vector<literal> literals) : literals(literals) {}
  bool subsumes(const clause& c) const;
  bool subsumes(const clause& c, literal l) const;
  bool operator == (const clause& c) const { return literals == c.literals; }
  bool contains(literal l) const;
  std::vector<literal>::const_iterator begin() const { return literals.begin(); }
  std::vector<literal>::const_iterator end() const { return literals.end(); }
  std::size_t width() const { return literals.size(); }
  struct domain_iterator : public std::vector<literal>::const_iterator {
  domain_iterator(std::vector<literal>::const_iterator it) : std::vector<literal>::const_iterator(it) {}
    variable operator * () const {
      return variable(std::vector<literal>::const_iterator::operator *());
    }
  };
  domain_iterator dom_begin() const { return literals.begin(); }
  domain_iterator dom_end() const { return literals.end(); }
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
  std::map<int,std::string> variable_names;
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
  proof_clause(const clause& c) : c(c) {}
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
  proof(std::vector<proof_clause>&& formula,
        std::list<proof_clause>&& resolution) :
  formula(std::move(formula)), resolution(std::move(resolution)) {}
};

#pragma once

#include <vector>
#include <string>
#include <list>
#include <map>

struct literal {
  int l;
  literal(int variable, bool polarity) : l(variable<<1) {
    if (polarity) l|=1;
  }
  bool operator == (const literal& a) const { return l==a.l; }
  bool opposite(const literal& a) const { return (l^a.l) == 1; }
  literal operator ~ () const { return l^1; }
  uint variable() const { return l >> 1; }
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

inline literal from_dimacs(int x) {
  if (x>0) return literal(x-1,true);
  else return literal(-x-1,false);
}

struct clause {
  std::vector<literal> literals;
  bool subsumes(const clause& c) const;
};
clause resolve(const clause& c, const clause& d, uint x);
clause resolve(const clause& c, const clause& d);

struct cnf {
  std::vector<clause> clauses;
  int variables;
  std::map<int,std::string> variable_names;
};

struct proof_clause {
  clause c;
  std::vector<const proof_clause*> derivation;
  proof_clause(clause c_) : c(c_) {}
  void resolve(const proof_clause& d, int x) {
    c = ::resolve(c, d.c, x);
    derivation.push_back(&d);
  }
};

struct proof {
  const std::vector<proof_clause>& formula;
  const std::list<proof_clause>& proof;
};





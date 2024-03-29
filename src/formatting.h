#pragma once

#include "data_structures.h"

#include <vector>
#include <string>
#include <sstream>
#include <cassert>
#include <unordered_map>

struct pretty_ {
  std::unordered_map<int,std::string> variable_names;
  std::unordered_map<std::string,int> name_variables;
  enum {
    TERMINAL,
    LATEX,
    DIMACS,
  } mode;
  std::string lor;
  std::string bot;
  pretty_() {}
  pretty_(const cnf& f) : variable_names(f.variable_names) {
    for (int i=0; i<f.variables; ++i) {
      if (variable_names.count(i)==0) {
        std::stringstream ss;
        ss << i+1;
        variable_names[i]=ss.str();
      }
    }
    for(const auto& it: variable_names) {
      name_variables[it.second]=it.first;
    }
  }
  std::ostream& operator << (variable x) {
    assert(x<variable_names.size());
    if (mode == DIMACS) return (*o) << (x+1);
    return (*o) << variable_names[x];
  }
  std::ostream& operator << (literal l) {
    assert(variable(l)<variable_names.size());
    if (mode == LATEX) {
      if (not l.polarity()) (*o) << "\\overline{";
      (*this) << variable(l);
      if (not l.polarity()) (*o) << "}";
    }
    else if (mode == DIMACS) {
      (*o) << (l.polarity()?" ":" -");
      (*this) << variable(l);
    }
    else {
      (*o) << (l.polarity()?' ':'~');
      (*this) << variable(l);
    }
    return (*o);
  }
private:
  std::ostream* o;
  friend pretty_& operator << (std::ostream& o, pretty_& p);
};
inline pretty_& operator << (std::ostream& o, pretty_& p) { p.o=&o; return p; }
extern pretty_ pretty;

inline std::ostream& operator << (std::ostream& o, literal l) {
  return o << pretty << l;
}
inline std::ostream& operator << (std::ostream& o, const clause& c) {
  if (pretty.mode == pretty.LATEX) o << '$';
  bool first=true;
  for (auto l:c) {
    if (first) first=false;
    else o << pretty.lor;
    o << l;
  }
  if (first) o << pretty.bot;
  if (pretty.mode == pretty.LATEX) o << '$';
  if (pretty.mode == pretty.DIMACS) o << " 0";
  return o;
}
inline std::ostream& operator << (std::ostream& o, const cnf& f) {
  for (auto& c:f.clauses) o << c << std::endl;
  return o;
}
inline std::ostream& operator << (std::ostream&o , const std::vector<int>& a) {
  char what[] = "-?+";
  for (size_t i=0;i<a.size();++i) o << what[a[i]+1] << pretty << i << " ";
  return o;
}

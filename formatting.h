#include "data_structures.h"

#include <vector>
#include <string>
#include <sstream>
#include <cassert>
#include <map>

struct pretty_ {
  std::map<int,std::string> variable_names;
  std::map<std::string,int> name_variables;
  bool latex;
  pretty_() : latex(false) {}
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
  std::ostream& operator << (unsigned int x) {
    assert(x<variable_names.size());
    return (*o) << variable_names[x];
  }
  std::ostream& operator << (literal l) {
    assert(l.variable()<variable_names.size());
    if (latex) {
      if (not l.polarity()) (*o) << "\\overline{";
      (*o) << variable_names[l.variable()];
      if (not l.polarity()) (*o) << "}";
      return (*o);
    }
    return (*o) << (l.polarity()?' ':'~') << variable_names[l.variable()];
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
  if (pretty.latex) o << '$';
  for (auto l:c) o << l;
  if (pretty.latex) o << '$';
  return o;
}
inline std::ostream& operator << (std::ostream& o, const cnf& f) {
  for (auto& c:f.clauses) o << c << std::endl;
  return o;
}
inline std::ostream& operator << (std::ostream&o , const std::vector<int>& a) {
  char what[] = "-?+";
  for (unsigned int i=0;i<a.size();++i) o << what[a[i]+1] << pretty << i << " ";
  return o;
}

#include "data_structures.h"

#include <vector>
#include <string>
#include <sstream>
#include <cassert>

struct pretty_ {
  std::vector<std::string> variable_names;
  std::ostream* o;
  pretty_() {}
  pretty_(const cnf& f) : variable_names(f.variable_names) {
    if (variable_names.empty()) {
      variable_names.reserve(f.variables);
      for (int i=0; i<f.variables; ++i) {
        std::stringstream ss;
        ss << i+1;
        variable_names.push_back(ss.str());
      }
    }
  }
  std::ostream& operator << (uint x) {
    assert(x<variable_names.size());
    return (*o) << variable_names[x];
  }
  std::ostream& operator << (literal l) {
    assert(l.variable()<variable_names.size());
    return (*o) << (l.polarity()?' ':'~') << variable_names[l.variable()];
  }
};
inline pretty_& operator << (std::ostream& o, pretty_& p) { p.o=&o; return p; }
extern pretty_ pretty;

inline std::ostream& operator << (std::ostream& o, literal l) {
  return o << pretty << l;
}
inline std::ostream& operator << (std::ostream& o, const clause& c) {
  for (auto l:c.literals) o << l;
  return o;
}
inline std::ostream& operator << (std::ostream& o, const cnf& f) {
  for (auto& c:f.clauses) o << c << std::endl;
  return o;
}
inline std::ostream& operator << (std::ostream&o , const std::vector<int>& a) {
  char what[] = "-?+";
  for (uint i=0;i<a.size();++i) o << what[a[i]+1] << pretty << i << " ";
  return o;
}

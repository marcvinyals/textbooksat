#include "data_structures.h"

#include <algorithm>
#include <cassert>
#include <set>
using namespace std;

bool clause::subsumes(const clause& c) const {
  assert(is_sorted(literals.begin(), literals.end()));
  assert(is_sorted(c.literals.begin(), c.literals.end()));
  return includes(c.literals.begin(), c.literals.end(), literals.begin(), literals.end());
}

bool clause::subsumes(const clause& c, literal l) const {
  assert(is_sorted(literals.begin(), literals.end()));
  assert(is_sorted(c.literals.begin(), c.literals.end()));
  for (auto it = c.begin(), jt = begin(); jt!=end(); ++it) {
    if (*jt==l) {
      ++jt;
      continue;
    }
    if (it == c.end() or *jt < *it) return false;
    if (not (*it < *jt)) ++jt;
  }
  return true;
}

bool clause::contains(literal l) const {
  assert(is_sorted(literals.begin(), literals.end()));
  return binary_search(literals.begin(), literals.end(), l);
}

clause resolve(const clause& c, const clause& d, uint x) {
  // TODO: make linear
  set<literal> ret;
  int found = 0;
  for (auto l:c) {
    if(l.variable()!=x) ret.insert(l);
    else found += l.polarity()+1;
  }
  for (auto l:d) {
    if(l.variable()!=x) ret.insert(l);
    else found += l.polarity()+1;
  }
  assert(found==3);
  return vector<literal>(ret.begin(), ret.end());
}

clause resolve(const clause& c, const clause& d) {
  // TODO: make linear
  set<literal> ret(c.begin(), c.end());
  int found = 0;
  for (auto l:d) {
    auto it=ret.find(~l);
    if (it!=ret.end()) {
      ret.erase(it);
      found++;
    }
    else {
      ret.insert(l);
    }
  }
  assert(found==1);
  return vector<literal>(ret.begin(), ret.end());
}

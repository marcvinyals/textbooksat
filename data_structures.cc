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

clause resolve(const clause& c, const clause& d) {
  vector<literal> ret;
  assert(c.width() and d.width());
  ret.reserve(c.width()+d.width()-1);
  int found=0;
  auto it=c.begin(), jt=d.begin();
  for (;it!=c.end() and jt!=d.end();) {
    if (*it==~*jt) {
      ++it;
      ++jt;
      ++found;
      continue;
    }
    if (*it<*jt) {
      ret.push_back(*it);
      ++it;
    }
    else {
      if (*it==*jt) ++it;
      ret.push_back(*jt);
      ++jt;
    }
  }
  copy(it,c.end(),back_inserter(ret));
  copy(jt,d.end(),back_inserter(ret));
  assert(found==1);
  return ret;
}

clause resolve(const clause& c, const clause& d, variable x) {
  return resolve(c,d);
}

clause reference_resolve(const clause& c, const clause& d, variable x) {
  set<literal> ret;
  int found = 0;
  for (auto l:c) {
    if(variable(l)!=x) ret.insert(l);
    else found += l.polarity()+1;
  }
  for (auto l:d) {
    if(variable(l)!=x) ret.insert(l);
    else found += l.polarity()+1;
  }
  assert(found==3);
  return vector<literal>(ret.begin(), ret.end());
}

clause reference_resolve(const clause& c, const clause& d) {
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

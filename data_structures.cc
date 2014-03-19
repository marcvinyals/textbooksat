#include "data_structures.h"

#include <unordered_set>
using namespace std;

bool clause::subsumes(const clause& c) const {
  unordered_set<literal> l2(c.literals.begin(),c.literals.end());
  for (auto l:literals) if (not l2.count(l)) return false;
  return true;
}

clause resolve(const clause& c, const clause& d, int x) {
  unordered_set<literal> ret;
  for (auto l:c.literals) if(l.variable()!=x) ret.insert(l);
  for (auto l:d.literals) if(l.variable()!=x) ret.insert(l);
  return clause({vector<literal>(ret.begin(), ret.end())});
}

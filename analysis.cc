#include "analysis.h"

#include <unordered_map>
#include <cassert>
#include <sstream>
#include <iostream>

using namespace std;

void measure(const vector<proof_clause>& formula,
             const list<proof_clause>& proof) {
  int length=0;
  unordered_map<const proof_clause*, int> last_used;
  int t=0;
  for (auto& c:proof) {
    length+=c.derivation.size();
    for (auto d:c.derivation) last_used[d]=t;
    ++t;
  }
  for (auto& c:formula) last_used.erase(&c);
  vector<int> remove_n(t);
  for (auto& kv:last_used) remove_n[kv.second]++;
  int space=0;
  int in_use=0;
  t=0;
  for (auto& c:proof) {
    in_use += last_used.count(&c);
    space = max(space, in_use);
    in_use -= remove_n[t];
    ++t;
  }
  assert(in_use==0);
  cerr << "Length " << length << endl;
  cerr << "Space " << space << endl;
}

void draw(std::ostream& out,
          const vector<proof_clause>& formula,
          const list<proof_clause>& proof) {
  out << "digraph G {" << endl;
  for (auto& c:proof) {
    vector<string> lemma_names;
    for (int i=0;i<c.derivation.size()-1;++i) {
      stringstream ss;
      ss << "lemma" << uint64_t(&c);
      if (i<c.derivation.size()-2) ss << "d" << i;
      if (i) out << lemma_names.back() << " -> " << ss.str() << endl;
      lemma_names.push_back(ss.str());
    }
    int i=-1;
    for (auto it=c.derivation.begin();it!=c.derivation.end();++it,++i) {
      out << "lemma" << uint64_t(*it) << " -> " << lemma_names[max(i,0)] << endl;      
    }
  }
  out << "}" << endl;
}

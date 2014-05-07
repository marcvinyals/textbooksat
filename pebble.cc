#include "pebble.h"

#include <algorithm>
#include <sstream>

#include "cdcl.h"
#include "ui.h"

using namespace std;

vector<int> parse_sequence(istream& in) {
  vector<int> pebble_sequence;
  string line;
  getline(in, line);
  stringstream ss(line);
  int u;
  while (ss >> u) pebble_sequence.push_back(u);
  return pebble_sequence;
}

vector<int> topo_sequence(const vector<vector<int>>& g) {
  vector<int> pebble_sequence;
  for (size_t i=0; i<g.size(); ++i) {
    if (g[i].empty()) continue;
    pebble_sequence.push_back(i+1);
    //if (i-7>0 and not g[i-7].empty()) pebble_sequence.push_back(-(i-7));
  }
  return pebble_sequence;
}

ostream& operator << (ostream& o, const vector<int>& v) {
  for (const auto& i:v) o << i << ' ';
  return o;
}

vector<int> bitreversal_sequence(const vector<vector<int>>& g) {
  vector<int> pebble_sequence;
  pebble_sequence.push_back(g.size()/2+1);
  for (size_t i=g.size()/2+1; i<g.size(); ++i) {
    int jtill=-1;
    for (int k : g[i]) if (k<g.size()/2) jtill=k;
    assert(jtill>=0);
    for (size_t j=1;j<=jtill;++j) {
      pebble_sequence.push_back(j+1);
      if (j>1) pebble_sequence.push_back(-j);
    }
    pebble_sequence.push_back(i+1);
    if (i+1==g.size()) break;
    pebble_sequence.push_back(-i);
    pebble_sequence.push_back(-jtill-1);
  }
  cerr << pebble_sequence << endl;
  return pebble_sequence;
}

pebble::pebble(std::istream& graph, const std::string& fn, int arity) :
  g(parse_kth(graph)), rg(g.size()), sub(fn, arity), expect(g.size()) {
  for (size_t i=0; i<g.size(); ++i) {
    for (int j:g[i]) {
      cerr << i << " " << j << endl;
      rg[j].push_back(i);
    }
  }
  vector<int> pebble_sequence;
  //pebble_sequence = parse_sequence(cin);
  if (pebble_sequence.empty()) pebble_sequence = bitreversal_sequence(g);
  for (int u : pebble_sequence) {
    if (u<0) {
      u=-1-u;
      assert(u>=0);
      assert(u<g.size());
      pebble_queue.push_back({u,-1});
    }
    else {
      --u;
      assert(u>=0);
      assert(u<g.size());
      pebble_queue.push_back({u,1});
      for (int pred : g[u]) {
        pebble_queue.push_back({pred,2});
      }
    }
  }
}

void pebble::bindto(cdcl& solver) {
  this->solver = &solver;
  solver.decide_plugin = bind(&pebble::get_decision, this);
}

void pebble::prepare_successors() {
  cerr << "Preparing things just in case" << endl;

  vector<int> nextusage(g.size(),-1);
  for (pair<int,int> p : pebble_queue) {
    int u = p.first;
    int exp = p.second;
    if (truth[u]>=exp) continue;
    for (int pred : g[u]) {
      if (nextusage[pred]==-1) nextusage[pred]=u;
    }
  }
  
  vector<int> busy(g.size());
  const vector<int>& a = solver->assignment;

  for (pair<int,int> p : pebble_queue) {
    if (p.second<0) continue;
    int u = p.first;
    if (busy[u]) continue;
    busy[u]=1;
    if (g[u].size()==1) { // Should test any subset of inputs, actually.
      int pred = g[u].front();
      if (nextusage[pred] != u) continue;
    }
    if (truth[u]) continue; // Should do more generic "will I get true by mistake?"
    cerr << "Preparing " << u+1 << " for " << nextusage[u]+1 << endl;
    vector<int> a_u(a.begin()+sub.arity*u, a.begin()+sub.arity*(u+1));
    if (not sub.false_assignments.count(a_u)) {
      decision_queue.push_front(literal(u*2,false));
      decision_queue.push_front(literal(u*2+1,false));
    }
  }
}

void pebble::cleanup() {
  cerr << "CLEANUP" << endl;
  solver->forget_wide(2);
  for (unsigned u = 0; u<g.size(); ++u) {
    if (expect[u]==0 and truth[u]) {
      bool ok = true;
      for (int succ : rg[u]) {
        if (expect[succ] and truth[succ]<2) {
          cerr << "Not erasing " << u+1 << " because of " << succ+1 << " which is " << truth[succ] << " but should be " << expect[succ] << endl;
          ok = false;
          break;
        }
      }
      if (ok) solver->forget_domain({u*2,u*2+1});
    }
  }
  update_truth();
}

bool pebble::is_complete(int u) const {
  if (truth[u]<1) return false;
  if (truth[u]==2) return true;
  for (int pred : g[u]) if (truth[pred]<2) return false;
  return true;
}


void pebble::pebble_next2() {
  static int uh = 0;
  cerr << "@ pebble_next2" << endl;
  for (auto it = pebble_queue.begin(); it != pebble_queue.end(); ++it) {
    int u = it->first;
    int exp = it->second;
    cerr << u+1 << "->" << exp << "  ";
  }
  cerr << endl;
  string line;
  //getline(cin, line);
  for (auto it = pebble_queue.begin(); it != pebble_queue.end(); ++it) {
    int u = it->first;
    int exp = it->second;
    cerr << "Considering to set " << u+1 << " to " << exp << endl;
    if (exp<0) {
      if (it==pebble_queue.begin()) {
        cerr << endl << "Erasing " << u+1 << endl << endl;
        uh=0;
        expect[u]=0;
        pebble_queue.pop_front();
        cleanup();
        return pebble_next2();
      }
      continue;
    }
    if (it==pebble_queue.begin()) {
      expect[u]=max(expect[u], exp);
    }
    if (exp==2 and not is_complete(u)) {
      continue;
    }
    else if (truth[u]>=exp) {
      if (not is_complete(u)) continue;
      uh=0;
      if (it==pebble_queue.begin()) {
        pebble_queue.pop_front();
        cleanup();
        prepare_successors();
        return pebble_next2();
      }
    }
    else {
      ++uh;
      if (uh>100) {
        cerr << "I think I am stuck" << endl;
        pebble_queue.clear();
        decision_queue.clear();
        return;
      }
      cerr << "Trying to get " << u+1 << " to " << exp << endl;
      const vector<int>& a = solver->assignment;
      vector<int> a_u(a.begin()+sub.arity*u, a.begin()+sub.arity*(u+1));
      if (sub.true_assignments.count(a_u)) {
        cerr << "Better do something else" << endl;
        continue;
      }
      if (not sub.false_assignments.count(a_u)) {
        if (exp==1) {
          decision_queue.push_back(literal(u*sub.arity, false));
          decision_queue.push_back(literal(u*sub.arity+1, false));
        }
        else if (exp==2) {
          decision_queue.push_back(literal(u*sub.arity, true));
          decision_queue.push_back(literal(u*sub.arity+1, true));
        }
        else {
          cerr << "bug" << endl;
          exit(1);
        }
        return;
      }
      cerr << "Now messing with its predecessors" << endl;
      for (int pred : g[u]) {
        if (truth[pred]<2) continue;
        vector<int> a_pred(a.begin()+sub.arity*pred, a.begin()+sub.arity*(pred+1));
        if (not sub.true_assignments.count(a_pred)) {
          decision_queue.push_back(literal(pred*sub.arity, false));
          return;
        }
      }
      cerr << "But found nothing to do, trying something else" << endl;
      continue;
      return;
    }
  }
}

void pebble::update_truth() {
  truth.assign(g.size(), 0);
  for (const restricted_clause r : solver->working_clauses) {
    int key, value;
    if (sub.is_clause(r.source->c, key, value)) truth[key]+=value;
  }
}

literal_or_restart pebble::get_decision() {
  cerr << "@ get_decision" << endl;

  static size_t nlearnt = -1;
  size_t nlearnt_new = solver->learnt_clauses.size();
  if (nlearnt_new != nlearnt) {
    nlearnt = nlearnt_new;
    update_truth();
    if (nlearnt) decision_queue.clear();
    if (nlearnt == 0 or solver->working_clauses.back().source->c.width()<=2) {
      cleanup();
      prepare_successors();
    }
  }

  if (decision_queue.empty()) {
    pebble_next2();
  }
  if (decision_queue.empty()) {
    cerr << "Empty decision queue" << endl;
    ui::bindto(*solver);
    return solver->decide_plugin(*solver);
  }
  literal l = decision_queue.front();
  decision_queue.pop_front();
  if (solver->assignment[variable(l)]) return get_decision();
  return l;
}

#include "pebble.h"

#include <algorithm>
#include <sstream>

#include "cdcl.h"
#include "ui.h"

using namespace std;

pebble::pebble(std::istream& graph, const std::string& fn, int arity) :
  g(parse_kth(graph)), rg(g.size()), sub(fn, arity), expect(g.size()) {
  for (size_t i=0; i<g.size(); ++i) {
    for (int j:g[i]) {
      cerr << i << " " << j << endl;
      rg[j].push_back(i);
    }
  }
  vector<int> pebble_sequence;
  pebble_sequence = parse_sequence(cin);
  if (pebble_sequence.empty()) {
    for (size_t i=0; i<g.size(); ++i) {
      if (g[i].empty()) continue;
      pebble_sequence.push_back(i);
      //if (i-7>0 and not g[i-7].empty()) pebble_sequence.push_back(-(i-7));
    }
  }
  for (int u : pebble_sequence) {
    if (u<0) {
      pebble_queue.push_back({-u,-1});
    }
    else {
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

vector<int> pebble::parse_sequence(istream& in) {
  vector<int> pebble_sequence;
  string line;
  getline(in, line);
  stringstream ss(line);
  int u;
  while (ss >> u) {
    if (u>0) pebble_sequence.push_back(u-1);
    else pebble_sequence.push_back(u+1);
  }
  return pebble_sequence;
}

void pebble::prepare_successors() {
  vector<int> busy(truth);
  for (pair<int,int> p : pebble_queue) {
    if (p.second!=1) continue;
    int u = p.first;
    if (u<0) continue;
    if (busy[u]) continue;
    bool waitforit = false;
    if (g[u].size()==1) {
      int pred = g[u].front();
      for (int sibling : rg[pred]) {
        if(busy[sibling]) {
          waitforit = true;
          break;
        }
      }
    }
    //waitforit=false;
    if (not waitforit) {
      decision_queue.push_front(literal(u*2,false));
      decision_queue.push_front(literal(u*2+1,false));
    }
    busy[u]=1;
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

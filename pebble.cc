#include "pebble.h"

#include <algorithm>
#include <sstream>

#include "cdcl.h"
#include "ui.h"

using namespace std;

pebble::pebble(std::istream& graph, const std::string& fn, int arity) :
  g(parse_kth(graph)), rg(g.size()), sub(fn, arity), expect(g.size()), skip(false) {
  for (size_t i=0; i<g.size(); ++i) {
    for (int j:g[i]) {
      cerr << i << " " << j << endl;
      rg[j].push_back(i);
    }
  }
  if(g.size()==16) {
  parse_sequence(cin);
  }
  else {
  for (size_t i=0; i<g.size(); ++i) {
    if (g[i].empty()) continue;
    pebble_sequence.push_back(i);
    //if (i-7>0 and not g[i-7].empty()) pebble_sequence.push_back(-(i-7));
  }
  }
  for (int u : pebble_sequence) {
    if (u<0) pebble_queue.push_back({-u,-1});
    else pebble_queue.push_back({u,1});
    expect[u]=1;
  }
}

void pebble::bindto(cdcl& solver) {
  this->solver = &solver;
  solver.decide_plugin = bind(&pebble::get_decision, this);
}

void pebble::parse_sequence(istream& in) {
  string line;
  getline(in, line);
  stringstream ss(line);
  int u;
  while (ss >> u) {
    if (u>0) pebble_sequence.push_back(u-1);
    else pebble_sequence.push_back(u+1);
  }
}

void pebble::prepare_successors() {
  vector<int> busy(truth);
  for (int u : pebble_sequence) {
    if (u<0) continue;
    if (busy[u]) continue;
    decision_queue.push_front(literal(u*2,false));
    decision_queue.push_front(literal(u*2+1,false));
    busy[u]=true;
  }
}

void pebble::prepare_successors(int who) {
  cerr << endl << "Preparing for " << who << endl << endl;
  vector<int> busy(truth);
  for (int u : pebble_sequence) {
    if (u<0) continue;
    if (find(g[u].begin(), g[u].end(), who) != g[u].end()) {
      if (busy[u]) break;
      //who=u;
      decision_queue.push_front(literal(u*2,false));
      decision_queue.push_front(literal(u*2+1,false));
      busy[u]=true;
    }
  }
}

/*void pebble::sequence_next() {
  if (not skip and not something_else.empty()) {
    pebble_queue.push_back(something_else.front());
    something_else.pop_front();
    prepare_successors();
    return;
  }
  if (pebble_sequence.empty()) return;
  int u = pebble_sequence.front();
  if (u<0) {
    if (skip) something_else.push_back(u);
    else expect[-u]=0;
    pebble_sequence.pop_front();
    return sequence_next();
  }
  skip = false;
  pebble_queue.push_back(u);
  pebble_sequence.pop_front();
  prepare_successors();
  }*/

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
}

/*void pebble::pebble_next() {
  cerr << "@ pebble_next" << endl;


  static int hm = 0;
  while(not pebble_queue.empty()) {
    if (truth[pebble_queue.front()] >= expect[pebble_queue.front()]) {
      pebble_queue.pop_front();
      hm=0;
    }
    else {
      ++hm;
      break;
    }
  }
  if (hm>10) {
    cerr << "I think I am stuck" << endl;
    pebble_sequence.clear();
    pebble_queue.clear();
    decision_queue.clear();
    return;
  }
  cleanup();

  if (pebble_queue.empty()) sequence_next();
  
  if (pebble_queue.empty()) {
    cerr << "Empty pebble queue" << endl;
    return;
  }
  int u = pebble_queue.front();
  cerr << "Trying to get " << u+1 << " to " << expect[u] << endl;

  const vector<int>& a = solver->assignment;
  vector<int> a_u(a.begin()+sub.arity*u, a.begin()+sub.arity*(u+1));
  if (sub.true_assignments.count(a_u)) {
    cerr << "Better do something else" << endl;
    something_else.push_back(pebble_queue.front());
    pebble_queue.pop_front();
    if (pebble_queue.empty()) skip = true;
    pebble_next();
  }
  
  if (expect[u]==1) {
    decision_queue.push_back(literal(u*sub.arity, false));
    decision_queue.push_back(literal(u*sub.arity+1, false));
  }
  else if (expect[u]==2) {
    decision_queue.push_back(literal(u*sub.arity, true));
    decision_queue.push_back(literal(u*sub.arity+1, true));
  }
  else {
    cerr << "bug" << endl;
    exit(1);
  }

  for (int pred : g[u]) {
    if (expect[pred] < 2) {
      expect[pred] = 2;
      pebble_queue.push_front(pred);
      cerr << "I should get " << pred+1 << "too" << endl;
      pebble_next();
    }
  }
  for (int pred : g[u]) {
    decision_queue.push_back(literal(pred*sub.arity,false));
  }
}*/

bool pebble::complete(int u) {
  if (truth[u]<1) return false;
  if (truth[u]==2) return true;
  for (int pred : g[u]) {
    if (truth[pred]==2) continue;
    if (expect[pred]<2) {
      expect[pred]=2;
      pebble_queue.push_front({pred,2});
      pebble_next2();
    }
    return false;
  }
  return true;
}

void pebble::pebble_next2() {
  string line;
  //getline(cin, line);
  static int uh = 0;
  cerr << "@ pebble_next2" << endl;
  for (auto it = pebble_queue.begin(); it != pebble_queue.end(); ++it) {
    int u = it->first;
    int exp = it->second;
    cerr << u << "->" << exp << "  ";
  }
  cerr << endl;
  for (auto it = pebble_queue.begin(); it != pebble_queue.end(); ++it) {
    int u = it->first;
    int exp = it->second;
    if (exp<0) {
      if (it==pebble_queue.begin()) {
        cerr << endl << "Erasing " << u << endl << endl;
        uh=0;
        expect[u]=0;
        pebble_queue.pop_front();
        if (pebble_sequence.front()==-u) pebble_sequence.pop_front();
        cleanup();
        return pebble_next2();
      }
    }
    else if (exp==2 and not complete(u)) {
      continue;
    }
    else if (truth[u]>=exp) {
      if (not complete(u)) continue;
      uh=0;
      if (it==pebble_queue.begin()) {
        pebble_queue.pop_front();
        if (pebble_sequence.front()==u) pebble_sequence.pop_front();
        return pebble_next2();
      }
    }
    else {
      ++uh;
      if (uh>20) {
        cerr << "I think I am stuck" << endl;
        pebble_sequence.clear();
        pebble_queue.clear();
        decision_queue.clear();
        return;
      }
      cerr << "trying to get " << u+1 << " to " << exp << endl;
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
      for (int pred : g[u]) {
        if (truth[pred]==2) continue;
        if (expect[pred]<2) {
          expect[pred]=2;
          pebble_queue.push_front({pred,2});
          return pebble_next2();
        }
      }
      for (int pred : g[u]) {
        vector<int> a_pred(a.begin()+sub.arity*pred, a.begin()+sub.arity*(pred+1));
        if (not sub.true_assignments.count(a_pred)) {
          decision_queue.push_back(literal(pred*sub.arity, false));
          return;
        }
      }
      continue;
      return;
    }
  }
}

literal_or_restart pebble::get_decision() {
  cerr << "@ get_decision" << endl;

  static size_t nlearnt = -1;
  size_t nlearnt_new = solver->learnt_clauses.size();
  if (nlearnt_new != nlearnt) {
    nlearnt = nlearnt_new;
    truth.assign(g.size(), 0);
    for (const restricted_clause r : solver->working_clauses) {
      int key, value;
      if (sub.is_clause(r.source->c, key, value)) truth[key]+=value;
    }
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

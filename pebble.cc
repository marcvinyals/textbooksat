#include "pebble.h"

#include "cdcl.h"
#include "ui.h"

using namespace std;

pebble::pebble(std::istream& graph, const std::string& fn, int arity) :
  g(parse_kth(graph)), sub(fn, arity), expect(g.size()) {
  for (size_t i=0; i<g.size(); ++i) {
    if (g[i].empty()) continue;
    pebble_queue.push_back(i);
    expect[i]=1;
  }
  for (int u : pebble_queue) {
    decision_queue.push_front(literal(u*2,false));
    decision_queue.push_front(literal(u*2+1,false));
  }
}


void pebble::pebble_next(cdcl& solver) {
  cerr << "@ pebble_next" << endl;
  solver.forget_wide(2);

  vector<int> truth(g.size());
  for (const restricted_clause r : solver.working_clauses) {
    int key, value;
    if (sub.is_clause(r.source->c, key, value)) truth[key]+=value;
  }

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
    int paco;
    cin >> paco;
    exit(1);
  }
  if (pebble_queue.empty()) {
    cerr << "Empty pebble queue" << endl;
    return;
  }
  int x = pebble_queue.front();  
  cerr << "Trying to get " << x << " to " << expect[x] << endl;
  if (expect[x]==1) {
    decision_queue.push_back(literal(x*sub.arity, false));
    decision_queue.push_back(literal(x*sub.arity+1, false));
  }
  else if (expect[x]==2) {
    decision_queue.push_back(literal(x*sub.arity, true));
    decision_queue.push_back(literal(x*sub.arity+1, true));
  }
  else {
    cerr << "bug" << endl;
    exit(1);
  }
  for (int pred : g[x]) {
    if (truth[pred] < 2) {
      expect[pred] = 2;
      pebble_queue.push_front(pred);
      return;
    }
  }
  for (int pred : g[x]) {
    decision_queue.push_back(literal(pred*sub.arity,false));
  }
}

literal_or_restart pebble::get_decision(cdcl& solver) {
  cerr << "@ get_decision" << endl;

  static size_t nlearnt = 0;
  size_t nlearnt_new = solver.learnt_clauses.size();
  if (nlearnt_new != nlearnt) {
    nlearnt = nlearnt_new;
    decision_queue.clear();
  }

  if (decision_queue.empty()) {
    pebble_next(solver);
  }
  if (decision_queue.empty()) {
    cerr << "Empty decision queue" << endl;
    ui::bindto(solver);
    return solver.decide_plugin(solver);
  }
  literal l = decision_queue.front();
  decision_queue.pop_front();
  if (solver.assignment[variable(l)]) return get_decision(solver);
  return l;
}

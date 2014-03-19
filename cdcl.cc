#include "cdcl.h"

#include <vector>
#include <set>
#include <deque>
#include <queue>
#include <list>
#include <algorithm>

#include <cassert>
#include <iostream>
using namespace std;

ostream& operator << (ostream& o, literal l) {
  return o << (l.polarity()?' ':'~') << l.variable()+1;
}
ostream& operator << (ostream& o, const clause& c) {
  for (auto l:c.literals) o << l;
  return o;
}
ostream& operator << (ostream& o, const cnf& f) {
  for (auto& c:f.clauses) o << c << endl;
  return o;
}

ostream& operator << (ostream&o , const vector<int>& a) {
  char what[] = "-?+";
  for (uint i=0;i<a.size();++i) o << what[a[i]+1] << i+1 << " ";
  return o;
}

clause resolve(const clause& c, const clause& d, int x) {
  set<literal> ret;
  for (auto l:c.literals) if(l.variable()!=x) ret.insert(l);
  for (auto l:d.literals) if(l.variable()!=x) ret.insert(l);
  return clause({vector<literal>(ret.begin(), ret.end())});
}

struct proof_clause {
  clause c;
  vector<const proof_clause*> derivation;
  proof_clause(clause c_) : c(c_) {}
  void resolve(const proof_clause& d, int x) {
    c = ::resolve(c, d.c, x);
    derivation.push_back(&d);
  }
};
ostream& operator << (ostream& o, const list<proof_clause>& v) {
  for (auto& i:v) o << "   " << i.c << endl;
  return o;
}

struct branch {
  literal to;
  const proof_clause* reason;
};
ostream& operator << (ostream& o, const branch& b) {
  return o << b.to.variable()+1 << ' ' << (b.reason?'=':'d') << ' ' << b.to.polarity() << "   ";
}

struct restricted_clause {
  vector<literal> literals;
  const proof_clause* source;
  int satisfied;
  restricted_clause(const proof_clause& c) :
    literals(c.c.literals), source(&c), satisfied(0) {}
  bool unit() const { return not satisfied and literals.size() == 1; }
  bool contradiction() const { return not satisfied and literals.empty(); }
  
  void restrict(literal l) {
    for (auto a = literals.begin(); a!= literals.end(); ++a) {
      if (*a==l) {
        satisfied++;
        break;
      }
      if (a->opposite(l)) {
        literals.erase(a);
        break;
      }
    }
  }

  void loosen(literal l) {
    for (auto a : source->c.literals) {
      if (a==l) {
        satisfied--;
        break;
      }
      if (a.opposite(l)) {
        literals.push_back(a);
        break;
      }      
    }
  }

  void restrict(const vector<int>& assignment) {
    literals.erase(remove_if(literals.begin(), literals.end(),
                             [this,&assignment](literal a) {
                               int al = assignment[a.variable()];
                               if (al) {
                                 if ((al==1)==a.polarity()) satisfied++;
                                 else return true;
                               }
                               return false;
                             }), literals.end());
  }
};
ostream& operator << (ostream& o, const restricted_clause& c) {
  for (auto l:c.literals) o << l;
  return o;
}
template<typename T>
ostream& operator << (ostream& o, const vector<T>& v) {
  for (auto& i:v) o << i;
  return o;
}
template<>
ostream& operator << (ostream& o, const vector<restricted_clause>& v) {
  for (auto& i:v) o << "   " << i << endl;
  return o;
}

struct propagation_queue_ {
  deque<branch> q;
  void propagate(const restricted_clause& c) {
    assert(c.unit());
    q.push_back({c.literals.front(),c.source});
  }
  void decide(literal l) {
    q.push_back({l,NULL});
  }
  void clear() {
    q.clear();
  }
  void pop() {
    q.pop_front();
  }
  bool empty() const {
    return q.empty();
  }
  const branch& front() const {
    return q.front();
  }
};


class cdcl {
 public:
  void solve(const cnf& f);

  function<literal(cdcl&)> decide_plugin;
  function<proof_clause(cdcl&, const vector<literal>::reverse_iterator&)> learn_plugin;
  static const bool backjump = true;

  literal decide_fixed();
  literal decide_ask();
  literal decide_reverse();

  proof_clause learn_fuip(const vector<literal>::reverse_iterator& first_decision);
  proof_clause learn_fuip_all(const vector<literal>::reverse_iterator& first_decision);

private:

  void unit_propagate();
  void learn();
  void decide();
  
  void unassign(literal l);
  vector<branch> build_branching_seq() const;
  
  bool solved; // Done
  const proof_clause* conflict; // Conflict

  vector<proof_clause> formula;
  list<proof_clause> learnt_clauses; // learnt clause db
  vector<restricted_clause> working_clauses; // clauses restricted to the current assignment

  vector<literal> branching_seq;
  vector<list<const proof_clause*>> reasons;
  propagation_queue_ propagation_queue;
  vector<int> assignment;
  
  set<int> decision_order;
  vector<bool> decision_polarity;
};

vector<branch> cdcl::build_branching_seq() const {
  vector<branch> ret;
  for (auto b:branching_seq) ret.push_back({b,(const proof_clause*)reasons[b.l].size()});
  return ret;
}

void cdcl::solve(const cnf& f) {
  cerr << f << endl;
  cerr << "Solving a formula with " << f.variables << " variables and " << f.clauses.size() << " clauses" << endl;

  for (int i=0; i<f.variables; ++i) decision_order.insert(i);
  decision_polarity.assign(f.variables,false);

  assignment.assign(f.variables,0);
  reasons.resize(f.variables*2);
  formula.reserve(f.clauses.size());
  for (auto& c : f.clauses) {
    formula.push_back(c);
    working_clauses.push_back(formula.back());
  }
  for (auto& c : working_clauses) {
    if (c.unit()) propagation_queue.propagate(c);
  }

  // Main loop
  solved = false;
  while(not solved) {
    conflict = NULL;
    while(not propagation_queue.empty()) {
      unit_propagate();
      if (conflict) {
        learn();
        if (solved) {
          cout << "UNSAT" << endl;
          return;
        }
      }
    }
    decide();
    if (solved) cout << "SAT" << endl;
  }
}

void cdcl::unit_propagate() {
  literal l = propagation_queue.front().to;
  cerr << "Unit propagating " << l;
  if (propagation_queue.front().reason)
    cerr << " because of " << *propagation_queue.front().reason;
  cerr << endl;
  auto& al = assignment[l.variable()];
  if (propagation_queue.front().reason)
    reasons[l.l].push_back(propagation_queue.front().reason);
  propagation_queue.pop();
  if (al) {
    // A literal may be propagated more than once for different
    // reasons. We keep a list of them.
    assert(l.polarity()==(al==1));
    return;
  }
  al = (l.polarity()?1:-1);
 
  decision_order.erase(l.variable());
  branching_seq.push_back(l);
  cerr << "Branching " << build_branching_seq() << endl;

  // Restrict unit clauses and check for conflicts and new unit
  // propagations.
  for (auto& c : working_clauses) {
    bool wasunit = c.unit();
    c.restrict(l);
    if (c.contradiction()) {
      cerr << "Conflict" << endl;
      conflict=c.source;
    }
    if (c.unit() and not wasunit) {
      cerr << c << " is now unit" << endl;
      propagation_queue.propagate(c);
    }
  }
}

void cdcl::unassign(literal l) {
  cerr << "Backtracking " << l << endl;
  for (auto& c : working_clauses) c.loosen(l);
  assignment[l.variable()] = 0;
  decision_order.insert(l.variable());
}

proof_clause cdcl::learn_fuip_all(const vector<literal>::reverse_iterator& first_decision) {
  vector<clause> q;
  q.push_back(conflict->c);
  for (auto it = branching_seq.rbegin(); it!= first_decision; ++it) {
    literal l = *it;
    vector<clause> qq;
    for (auto& c:q) {
      for (auto d:reasons[l.l]) {
        qq.push_back(resolve(c,d->c,l.variable()));
        cerr << "Resolved " << c << " with " << *d << " and got " << qq.back() << endl;
        restricted_clause r(qq.back());
        r.restrict(assignment);
        if ((solved and r.contradiction()) or (not solved and r.unit())) return qq.back();
      }
    }
    swap(q,qq);
  }
  assert(false);
}

// Find the learnt clause by resolving the conflict clause with
// reasons for propagation until the result becomes asserting (unit
// after restriction). The result is an input regular resolution
// proof.
proof_clause cdcl::learn_fuip(const vector<literal>::reverse_iterator& first_decision) {
  proof_clause c(*conflict);
  c.derivation.push_back(conflict);
  cerr << "Conflict clause " << c << endl;
  cerr << "Assignment " << assignment << endl;
  for (auto it = branching_seq.rbegin(); it!=first_decision; ++it) {
    literal l = *it;
    restricted_clause d(c);
    d.restrict(assignment);
    if (not solved and d.unit()) break;
    c.resolve(*reasons[l.l].front(), l.variable());
    cerr << "Resolved with " << *reasons[l.l].front() << " and got " << c << endl;
  }
  return c;
}
  
void cdcl::learn() {
  cerr << "Branching " << build_branching_seq() << endl;
  // Backtrack to first decision level.
  auto first_decision = branching_seq.rbegin();
  for (;first_decision != branching_seq.rend(); ++first_decision) {
    unassign(*first_decision);
    if (reasons[first_decision->l].empty()) break;
  }
  cerr << "Branching " << build_branching_seq() << endl;

  // If there is no decision on the stack, the formula is unsat.
  if (first_decision == branching_seq.rend()) solved = true;

  proof_clause learnt_clause = learn_plugin(*this, first_decision);
  cerr << "Learning clause " << learnt_clause << endl;
  learnt_clauses.push_back(learnt_clause);
  
  if (solved) {
    cerr << "I learned the following clauses:" << endl << learnt_clauses << endl;
    return;
  }

  // Now keep backtracking while the clause is unit
  auto backtrack_limit = first_decision;
  for (++backtrack_limit; backtrack_limit != branching_seq.rend(); ++backtrack_limit) {
    if (not backjump) break;
    bool found = false;
    for (auto l:learnt_clause.c.literals) if (l.opposite(*backtrack_limit)) found = true;
    if (found) break;
  }

  // But always backtrack to a decision
  while(not reasons[backtrack_limit.base()->l].empty()) --backtrack_limit;

  // Complete backtracking
  for (auto it=first_decision+1; it!=backtrack_limit; ++it) {
    unassign(*it);
  }
  for (auto it=backtrack_limit.base(); it!=branching_seq.end(); ++it) {
    reasons[it->l].clear();
  }
  branching_seq.erase(backtrack_limit.base(),branching_seq.end());
  
  // Add the learnt clause to working clauses and immediately start
  // propagating
  working_clauses.push_back(learnt_clauses.back());
  working_clauses.back().restrict(assignment);
  assert(working_clauses.back().unit());
  cerr << "Branching " << build_branching_seq() << endl;
  propagation_queue.clear();
  propagation_queue.propagate(working_clauses.back());
  conflict = NULL;
}

void cdcl::decide() {
  cerr << "Deciding something" << endl;
  if (decision_order.empty()) {
    solved=true;
    return;
  }
  literal decision = decide_plugin(*this);
  propagation_queue.decide(decision);
}

literal cdcl::decide_fixed() {
  int decision_variable = *decision_order.begin();
  decision_order.erase(decision_order.begin());
  return literal(decision_variable,decision_polarity[decision_variable]);
}

literal cdcl::decide_reverse() {
  auto back = --decision_order.end();
  int decision_variable = *back;
  decision_order.erase(back);
  return literal(decision_variable,decision_polarity[decision_variable]);
}

literal cdcl::decide_ask() {
  cout << "Good day oracle, would you mind giving me some advice?" << endl;
  cout << "This is the branching sequence so far: " << build_branching_seq() << endl;
  cout << "I learned the following clauses:" << endl << learnt_clauses << endl;
  cout << "Therefore these are the restricted clauses I have in mind:" << endl << working_clauses << endl;
  int dimacs_decision = 0;
  while (not dimacs_decision) {
    if (not cin) exit(1);
    cout << "Please input a literal in dimacs format." << endl;
    cin >> dimacs_decision;
  }
  return from_dimacs(dimacs_decision);
}

void cdcl_solver::solve(const cnf& f) {
  cdcl solver;
  if (decide == "fixed") solver.decide_plugin = &cdcl::decide_fixed;
  else if (decide == "ask") solver.decide_plugin = &cdcl::decide_ask;
  else if (decide == "reverse") solver.decide_plugin = &cdcl::decide_reverse;
  else {
    cerr << "Invalid decision procedure" << endl;
    exit(1);
  }
  if (learn == "1uip") solver.learn_plugin = &cdcl::learn_fuip;
  else if (learn == "1uip-all") solver.learn_plugin = &cdcl::learn_fuip_all;
  else {
    cerr << "Invalid learning scheme" << endl;
    exit(1);
  }
  solver.solve(f);
}

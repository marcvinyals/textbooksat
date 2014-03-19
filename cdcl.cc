#include "cdcl.h"

#include "color.h"

#include <vector>
#include <set>
#include <deque>
#include <queue>
#include <list>
#include <algorithm>
#include <unordered_map>

#include <sstream>
#include <cassert>
#include <iostream>
using namespace std;

struct pretty_ {
  vector<string> variable_names;
  ostream* o;
  pretty_() {}
  pretty_(const cnf& f) : variable_names(f.variable_names) {
    if (variable_names.empty()) {
      variable_names.reserve(f.variables);
      for (int i=0; i<f.variables; ++i) {
        stringstream ss;
        ss << i+1;
        variable_names.push_back(ss.str());
      }
    }
  }
  ostream& operator << (uint x) {
    assert(x<variable_names.size());
    return (*o) << variable_names[x];
  }
  ostream& operator << (literal l) {
    assert(l.variable()<variable_names.size());
    return (*o) << (l.polarity()?' ':'~') << variable_names[l.variable()];
  }
};
pretty_& operator << (ostream& o, pretty_& p) { p.o=&o; return p; }
static pretty_ pretty;

ostream& operator << (ostream& o, literal l) {
  return o << pretty << l;
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
  for (uint i=0;i<a.size();++i) o << what[a[i]+1] << pretty << i << " ";
  return o;
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
  return o << pretty << b.to.variable() << ' ' << (b.reason?'=':'d') << ' ' << b.to.polarity() << "   ";
}

struct literal_or_restart {
  literal l;
  bool restart;
  literal_or_restart(literal l_) : l(l_), restart(false) {}
  literal_or_restart(bool restart_) : l(0,0), restart(restart_) {}
};

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

  function<literal_or_restart(cdcl&)> decide_plugin;
  function<proof_clause(cdcl&, const vector<literal>::reverse_iterator&)> learn_plugin;

  static const bool config_backjump = false;
  static const bool config_minimize = true;

  literal_or_restart decide_fixed();
  literal_or_restart decide_ask();
  literal_or_restart decide_reverse();

  proof_clause learn_fuip(const vector<literal>::reverse_iterator& first_decision);
  proof_clause learn_fuip_all(const vector<literal>::reverse_iterator& first_decision);

private:

  void unit_propagate();
  void learn();
  void decide();
  void restart();
  
  void unassign(literal l);
  void backjump(const proof_clause& learnt_clause,
                const vector<literal>::reverse_iterator& first_decision,
                vector<literal>::reverse_iterator& backtrack_limit);
  void minimize(proof_clause& c) const;

  void analize() const;
  void draw() const;
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

  for (const auto& c : f.clauses) formula.push_back(c);
  
  for (int i=0; i<f.variables; ++i) decision_order.insert(i);
  decision_polarity.assign(f.variables,false);

  assignment.resize(f.variables,0);
  reasons.resize(f.variables*2);
  formula.reserve(f.clauses.size());
  restart();

  // Main loop
  solved = false;
  while(not solved) {
    conflict = NULL;
    while(not propagation_queue.empty()) {
      unit_propagate();
      if (conflict) {
        learn();
        if (solved) {
          cerr << "UNSAT" << endl;
          analize();
          draw();
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

// This is currently equivalent to 1UIP. It can be hacked to prompt
// the user for a choice.
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

// Clause minimization. Try eliminating literals from the clause by
// resolving them with a reason.
void cdcl::minimize(proof_clause& c) const {
  cerr << Color::Modifier(Color::FG_RED) << "Minimize:" << Color::Modifier(Color::FG_DEFAULT) << endl;
  restricted_clause d(c.c);
  d.restrict(assignment);
  if (d.contradiction()) return;
  literal asserting = d.literals.front();
  for (auto l:c.c.literals) {
    // We do not minimize asserting literals. We could be a bit bolder
    // here, but then we would require backjumps.
    if (l==asserting) continue;
    for (auto d:reasons[(~l).l]) {
      proof_clause cc(c);
      cc.resolve(*d,l.variable());
      cerr << "        Minimize? " << c.c << " vs " << *d << endl;
      if (cc.c.subsumes(c.c)) {
        cerr << Color::Modifier(Color::FG_GREEN) << "Minimize!" << Color::Modifier(Color::FG_DEFAULT) << endl;
        c=cc;
      }
    }
  }
}

void cdcl::backjump(const proof_clause& learnt_clause,
                    const vector<literal>::reverse_iterator& first_decision,
                    vector<literal>::reverse_iterator& backtrack_limit) {
  // We want to backtrack while the clause is unit
  for (; backtrack_limit != branching_seq.rend(); ++backtrack_limit) {
    bool found = false;
    for (auto l:learnt_clause.c.literals) if (l.opposite(*backtrack_limit)) found = true;
    if (found) break;
  }

  // But always backtrack to a decision
  while(not reasons[backtrack_limit.base()->l].empty()) --backtrack_limit;

  // Actually backtrack
  for (auto it=first_decision+1; it!=backtrack_limit; ++it) {
    unassign(*it);
  }
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

  if (config_minimize) minimize(learnt_clause);
  
  cerr << "Learning clause " << learnt_clause << endl;
  learnt_clauses.push_back(learnt_clause);
  
  if (solved) {
    cerr << "I learned the following clauses:" << endl << learnt_clauses << endl;
    return;
  }

  // Complete backtracking
  auto backtrack_limit = first_decision;
  backtrack_limit++;
  if (config_backjump) backjump(learnt_clause, first_decision, backtrack_limit);
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
  literal_or_restart decision = decide_plugin(*this);
  if (decision.restart) restart();
  else propagation_queue.decide(decision.l);
}

literal_or_restart cdcl::decide_fixed() {
  int decision_variable = *decision_order.begin();
  decision_order.erase(decision_order.begin());
  return literal(decision_variable,decision_polarity[decision_variable]);
}

literal_or_restart cdcl::decide_reverse() {
  auto back = --decision_order.end();
  int decision_variable = *back;
  decision_order.erase(back);
  return literal(decision_variable,decision_polarity[decision_variable]);
}

literal_or_restart cdcl::decide_ask() {
  cout << "Good day oracle, would you mind giving me some advice?" << endl;
  cout << "This is the branching sequence so far: " << build_branching_seq() << endl;
  cout << "I learned the following clauses:" << endl << learnt_clauses << endl;
  cout << "Therefore these are the restricted clauses I have in mind:" << endl << working_clauses << endl;
  int dimacs_decision = 0;
  while (not dimacs_decision) {
    if (not cin) {
      cerr << "No more input?" << endl;
      exit(1);
    }
    cout << "Please input either of:" << endl;
    cout << " * a literal in dimacs format" << endl;
    cout << " * an assignment <varname> {0,1}" << endl;
    cout << " * the keyword 'restart'" << endl;
    string in;
    cin >> in;
    if (in == "restart") return true;
    stringstream ss(in);
    ss >> dimacs_decision;
    if (dimacs_decision) break;
    int polarity;
    cin >> polarity;
    polarity = polarity*2-1;
    if (abs(polarity)>1) continue;
    auto it = find(pretty.variable_names.begin(), pretty.variable_names.end(), in);
    if (it == pretty.variable_names.end()) continue;
    dimacs_decision = polarity*((it-pretty.variable_names.begin())+1);
  }
  return from_dimacs(dimacs_decision);
}

void cdcl::restart() {
  cerr << "Restarting" << endl;
  for (auto l:branching_seq) decision_order.insert(l.variable());
  fill(assignment.begin(), assignment.end(), 0);
  fill(reasons.begin(), reasons.end(), list<const proof_clause*>());
  branching_seq.clear();
  propagation_queue.clear();
  working_clauses.clear();

  for (auto& c : formula) working_clauses.push_back(c);
  for (auto& c : learnt_clauses) working_clauses.push_back(c);
  for (auto& c : working_clauses) {
    if (c.unit()) propagation_queue.propagate(c);
  }
}

void cdcl::analize() const {
  int length=0;
  unordered_map<const proof_clause*, int> last_used;
  int t=0;
  for (auto& c:learnt_clauses) {
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
  for (auto& c:learnt_clauses) {
    in_use += last_used.count(&c);
    space = max(space, in_use);
    in_use -= remove_n[t];
    ++t;
  }
  assert(in_use==0);
  cerr << "Length " << length << endl;
  cerr << "Space " << space << endl;
}

void cdcl::draw() const {
  cout << "digraph G {" << endl;
  for (auto& c:learnt_clauses) {
    vector<string> lemma_names;
    for (int i=0;i<c.derivation.size()-1;++i) {
      stringstream ss;
      ss << "lemma" << uint64_t(&c);
      if (i<c.derivation.size()-2) ss << "d" << i;
      if (i) cout << lemma_names.back() << " -> " << ss.str() << endl;
      lemma_names.push_back(ss.str());
    }
    int i=-1;
    for (auto it=c.derivation.begin();it!=c.derivation.end();++it,++i) {
      cout << "lemma" << uint64_t(*it) << " -> " << lemma_names[max(i,0)] << endl;      
    }
  }
  cout << "}" << endl;
}

void cdcl_solver::solve(const cnf& f) {
  pretty = pretty_(f);
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

#include "cdcl.h"

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
#include <iomanip>

#include "formatting.h"
#include "color.h"
#include "log.h"

using namespace std;

ostream& operator << (ostream& o, const list<proof_clause>& v) {
  for (const auto& i:v) o << "   " << i.c << endl;
  return o;
};

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
    literals(c.begin(), c.end()), source(&c), satisfied(0) {}
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
    for (auto a : source->c) {
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
  for (const auto& i:v) o << i;
  return o;
}
template<>
ostream& operator << (ostream& o, const vector<restricted_clause>& v) {
  for (size_t i = 0; i<v.size(); ++i) o << setw(5) << i << ": " << v[i] << endl;
  return o;
}

struct propagation_queue {
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
  proof solve(const cnf& f);

  function<literal_or_restart(cdcl&)> decide_plugin;
  function<proof_clause(cdcl&, const vector<literal>::reverse_iterator&)> learn_plugin;

  bool config_backjump;
  bool config_minimize;

  literal_or_restart decide_fixed();
  literal_or_restart decide_ask();
  literal_or_restart decide_reverse();

  proof_clause learn_fuip(const vector<literal>::reverse_iterator& first_decision);
  proof_clause learn_fuip_all(const vector<literal>::reverse_iterator& first_decision);

private:

  void unit_propagate();
  void learn();
  void forget(uint m);
  void decide();
  void restart();
  
  void assign(literal l);
  void unassign(literal l);
  void backjump(const proof_clause& learnt_clause,
                const vector<literal>::reverse_iterator& first_decision,
                vector<literal>::reverse_iterator& backtrack_limit);
  void minimize(proof_clause& c) const;

  vector<branch> build_branching_seq() const;
  bool consistent() const;
  
  bool solved; // Done
  const proof_clause* conflict; // Conflict

  // Copy of the formula.
  vector<proof_clause> formula;
  // Learnt clauses, in order. We will pointers to proof clauses, so
  // they should not be erased or reallocated.
  list<proof_clause> learnt_clauses;
  // Clauses restricted to the current assignment.
  vector<restricted_clause> working_clauses;

  // List of unit propagations, in chronological order.
  vector<literal> branching_seq;
  // Reasons for propagation, indexed by literal number. If a
  // propagated literal does not have any reason, then it was
  // decided. It is possible for a literal to have multiple reasons
  // before being propagated; we choose the first as the main reason.
  vector<list<const proof_clause*>> reasons;
  // Queue of literals waiting to be unit-propagated.
  struct propagation_queue propagation_queue;
  // Assignment induced by the branching sequence, indexed by variable
  // number. Possible values are 1 (true), -1 (false) or 0
  // (unassigned).
  vector<int> assignment;

  // Queue of variables waiting to be decided.
  set<int> decision_order;
  // Value a decided variable should be set to, indexed by variable
  // number.
  vector<bool> decision_polarity;
};

vector<branch> cdcl::build_branching_seq() const {
  vector<branch> ret;
  for (auto b:branching_seq) ret.push_back({b,reasons[b.l].empty()?NULL:reasons[b.l].front()});
  return ret;
}

bool cdcl::consistent() const {
  for (auto l:branching_seq) {
    int al = assignment[l.variable()];
    assert(al and l.polarity()==(al==1));
  }
  for (auto branch:propagation_queue.q) {
    int al = assignment[branch.to.variable()];
    assert((not al) or branch.to.polarity()==(al==1));
  }
  for (auto v:decision_order) {
    assert(not assignment[v]);
  }
  assert(assignment.size() <= branching_seq.size() + propagation_queue.q.size() + decision_order.size());
  return true;
}

proof cdcl::solve(const cnf& f) {
  LOG(LOG_STATE) << f << endl;
  LOG(LOG_ACTIONS) << "Solving a formula with " << f.variables << " variables and " << f.clauses.size() << " clauses" << endl;

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
    assert(consistent());
    while(not propagation_queue.empty()) {
      unit_propagate();
      if (conflict) {
        learn();
        if (solved) {
          LOG(LOG_ACTIONS) << "UNSAT" << endl;
          return {formula,learnt_clauses};
        }
      }
    }
    decide();
    if (solved) cout << "SAT" << endl;
  }
  assert(false);
}

void cdcl::unit_propagate() {
  literal l = propagation_queue.front().to;
  LOG(1) << "Unit propagating " << l;
  if (propagation_queue.front().reason)
    LOG(1) << " because of " << *propagation_queue.front().reason;
  LOG(LOG_ACTIONS) << endl;

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
 
  branching_seq.push_back(l);
  LOG(LOG_STATE) << "Branching " << build_branching_seq() << endl;

  assign(l);
}

void cdcl::assign(literal l) {
  auto& al = assignment[l.variable()];
  assert(not al);
  al = (l.polarity()?1:-1);

  // Restrict unit clauses and check for conflicts and new unit
  // propagations.
  for (auto& c : working_clauses) {
    bool wasunit = c.unit();
    c.restrict(l);
    if (c.contradiction()) {
      LOG(LOG_STATE) << "Conflict" << endl;
      conflict=c.source;
    }
    if (c.unit() and not wasunit) {
      LOG(LOG_STATE) << c << " is now unit" << endl;
      propagation_queue.propagate(c);
    }
  }
  
  decision_order.erase(l.variable());
}

void cdcl::unassign(literal l) {
  LOG(LOG_STATE) << "Backtracking " << l << endl;
  auto& al = assignment[l.variable()];
  assert(al);
  al = 0;

  for (auto& c : working_clauses) c.loosen(l);
  decision_order.insert(l.variable());
}

// This is currently equivalent to 1UIP. It can be hacked to prompt
// the user for a choice.
proof_clause cdcl::learn_fuip_all(const vector<literal>::reverse_iterator& first_decision) {
  deque<clause> q;
  q.push_back(conflict->c);
  unordered_map<clause,pair<const clause*, const proof_clause*>> parent;
  parent[conflict->c]={NULL,conflict};
  while(not q.empty()) {
    clause c=q.front();q.pop_front();
    for (auto it = branching_seq.rbegin(); it!= first_decision; ++it) {
      literal l = *it;
      if (not c.contains(~l)) continue;
      for (auto d:reasons[l.l]) {
        clause e = resolve(c, d->c, l.variable());
        if (parent.count(e)) continue;
        //cerr << "Resolved " << c << " with " << *d << " and got " << q.back() << endl;
        restricted_clause r(e);
        r.restrict(assignment);
        q.push_back(e);
        parent[e]={&(parent.find(c)->first), d};
        if ((solved and r.contradiction()) or (not solved and r.unit())) {
          proof_clause ret(e);
          const clause* p = &e;
          while(p) {
            ret.derivation.push_back(parent[*p].second);
            p = parent[*p].first;
          }
          reverse(ret.derivation.begin(), ret.derivation.end());
          return ret;
        }
      }
    }
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
  LOG(LOG_STATE) << "Conflict clause " << c << endl;
  LOG(LOG_STATE) << "Assignment " << assignment << endl;
  for (auto it = branching_seq.rbegin(); it!=first_decision; ++it) {
    literal l = *it;
    assert (c.c.contains(~l));
    restricted_clause d(c);
    d.restrict(assignment);
    if (not solved and d.unit()) break;
    c.resolve(*reasons[l.l].front(), l.variable());
    LOG(LOG_DETAIL) << "Resolved with " << *reasons[l.l].front() << " and got " << c << endl;
  }
  return c;
}

// Clause minimization. Try eliminating literals from the clause by
// resolving them with a reason.
void cdcl::minimize(proof_clause& c) const {
  LOG(LOG_DETAIL) << Color::Modifier(Color::FG_RED) << "Minimize:" << Color::Modifier(Color::FG_DEFAULT) << endl;
  restricted_clause d(c.c);
  d.restrict(assignment);
  if (d.contradiction()) return;
  literal asserting = d.literals.front();
  for (auto l:c) {
    // We do not minimize asserting literals. We could be a bit bolder
    // here, but then we would require backjumps.
    if (l==asserting) continue;
    for (auto d:reasons[(~l).l]) {
      proof_clause cc(c);
      cc.resolve(*d,l.variable());
      LOG(LOG_DETAIL) << "        Minimize? " << c.c << " vs " << *d << endl;
      if (cc.c.subsumes(c.c)) {
        LOG(LOG_DETAIL) << Color::Modifier(Color::FG_GREEN) << "Minimize!" << Color::Modifier(Color::FG_DEFAULT) << endl;
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
    for (auto l:learnt_clause) if (l.opposite(*backtrack_limit)) found = true;
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
  LOG(LOG_STATE) << "Branching " << build_branching_seq() << endl;
  // Backtrack to first decision level.
  auto first_decision = branching_seq.rbegin();
  for (;first_decision != branching_seq.rend(); ++first_decision) {
    unassign(*first_decision);
    if (reasons[first_decision->l].empty()) break;
  }
  LOG(LOG_STATE) << "Branching " << build_branching_seq() << endl;

  // If there is no decision on the stack, the formula is unsat.
  if (first_decision == branching_seq.rend()) solved = true;

  assert(first_decision != branching_seq.rbegin());
  proof_clause learnt_clause = learn_plugin(*this, first_decision);

  if (config_minimize) minimize(learnt_clause);
  
  LOG(LOG_ACTIONS) << "Learning clause " << learnt_clause << endl;
  assert(not config_backjump or
         find_if(working_clauses.begin(), working_clauses.end(),
                 [learnt_clause] (const restricted_clause& i) {
                   return i.source->c == learnt_clause.c;
                 }) == working_clauses.end());
  learnt_clauses.push_back(learnt_clause);
  
  if (solved) {
    LOG(LOG_ACTIONS) << "I learned the following clauses:" << endl << learnt_clauses << endl;
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
  LOG(LOG_STATE) << "Branching " << build_branching_seq() << endl;
  // There may be a more efficient way to do this.
  propagation_queue.clear();
  for (const auto& c : working_clauses) {
    if (c.unit() and not assignment[c.literals.front().variable()]) {
      propagation_queue.propagate(c);
    }
  }
  
  conflict = NULL;
}

void cdcl::decide() {
  LOG(LOG_ACTIONS) << "Deciding something" << endl;
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
    cout << " * the keyword 'forget' and a restricted clause number" << endl;
    string in;
    cin >> in;
    if (in[0] == '#') {
      cin.ignore(numeric_limits<streamsize>::max(),'\n');
      continue; }
    if (in == "restart") return true;
    if (in == "forget") {
      uint m;
      cin >> m;
      if (m < formula.size()) {
        cerr << "Refusing to forget an axiom" << endl;
        continue;
      }
      if (m >= working_clauses.size()) {
        cerr << "I cannot forget something I have not learnt yet" << endl;
        continue;
      }
      forget(m);
      return decide_ask();
    }
    auto it= pretty.name_variables.find(in);
    if (it == pretty.name_variables.end()) {
      stringstream ss(in);
      ss >> dimacs_decision;
    }
    else {
      int polarity;
      cin >> polarity;
      polarity = polarity*2-1;
      if (abs(polarity)>1) continue;
      dimacs_decision = polarity*( it->second + 1 );
    }
  }
  return from_dimacs(dimacs_decision);
}

void cdcl::restart() {
  LOG(LOG_ACTIONS) << "Restarting" << endl;
  for (auto l:branching_seq) decision_order.insert(l.variable());
  fill(assignment.begin(), assignment.end(), 0);
  fill(reasons.begin(), reasons.end(), list<const proof_clause*>());
  branching_seq.clear();
  propagation_queue.clear();
  working_clauses.clear();

  for (const auto& c : formula) working_clauses.push_back(c);
  for (const auto& c : learnt_clauses) working_clauses.push_back(c);
  for (const auto& c : working_clauses) {
    if (c.unit()) propagation_queue.propagate(c);
  }
}

void cdcl::forget(uint m) {
  assert (m>=formula.size());
  assert (m<working_clauses.size());
  auto& target = working_clauses[m];
  LOG(LOG_ACTIONS) << "Forgetting " << target << endl;
  auto branch_seq = build_branching_seq();
  for (const auto& branch : build_branching_seq()) {
    if (branch.reason == target.source) {
      LOG(LOG_ACTIONS) << target << " is used to propagate " << branch.to << "; refusing to forget it." << endl;
      return;
    }
  }
  working_clauses.erase(working_clauses.begin()+m);
}

proof cdcl_solver::solve(const cnf& f) {
  pretty = pretty_(f);
  static cdcl solver;
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
  solver.config_backjump = backjump;
  solver.config_minimize = minimize;
  return solver.solve(f);
}

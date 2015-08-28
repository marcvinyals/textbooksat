#include "cdcl.h"

#include <algorithm>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>

#include "formatting.h"
#include "color.h"
#include "log.h"
#include "ui.h"

using namespace std;

ostream& operator << (ostream& o, const list<proof_clause>& v) {
  for (const auto& i:v) o << "   " << i.c << endl;
  return o;
};
ostream& operator << (ostream& o, const branch& b) {
  return o << pretty << variable(b.to) << ' ' << (b.reason?'=':'d') << ' ' << b.to.polarity() << "   ";
}
ostream& operator << (ostream& o, const restricted_clause& c) {
  if (c.satisfied) o << Color::Modifier(Color::TY_CROSSED);
  for (auto l:c.literals) o << l;
  if (c.satisfied) o << Color::Modifier(Color::DEFAULT);
  return o;
}
ostream& operator << (ostream& o, const vector<clause>& v) {
  for (size_t i = 0; i<v.size(); ++i) o << setw(5) << i << ": " << v[i] << endl;
  return o;
}

void restricted_clause::restrict(literal l) {
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

void restricted_clause::loosen(literal l) {
  for (literal a : *source) {
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

void restricted_clause::restrict(const std::vector<int>& assignment) {
  literals.erase(remove_if(literals.begin(), literals.end(),
                           [this,&assignment](literal a) {
                             int al = assignment[variable(a)];
                             if (al) {
                               if ((al==1)==a.polarity()) satisfied++;
                               else return true;
                             }
                             return false;
                           }), literals.end());
}

void restricted_clause::reset() {
  literals.assign(source->begin(), source->end());
  satisfied = 0;
}


bool cdcl::variable_cmp_vsids(variable x, variable y) const {
  double d = variable_activity[y] - variable_activity[x];
  if (d) return d<0;
  return x < y;
}

bool cdcl::variable_cmp_fixed(variable x, variable y) const {
  return x < y;
}

bool cdcl::variable_cmp_reverse(variable x, variable y) const {
  return x > y;
}

bool cdcl::consistent() const {
  for (auto branch:branching_seq) {
    int al = assignment[variable(branch.to)];
    assert(al and branch.to.polarity()==(al==1));
  }
  for (auto branch:propagation_queue.q) {
    int al = assignment[variable(branch.to)];
    assert((not al) or branch.to.polarity()==(al==1));
  }
  for (auto v:decision_order) {
    assert(not assignment[v]);
  }
  assert(assignment.size() <= branching_seq.size() + propagation_queue.q.size() + decision_order.size());
  return true;
}



/*
 * Main
 */

proof cdcl::solve(const cnf& f) {
  LOG(LOG_STATE) << f << endl;
  LOG(LOG_ACTIONS) << "Solving a formula with " << f.variables << " variables and " << f.clauses.size() << " clauses" << endl;

  formula.reserve(f.clauses.size());
  for (const auto& c : f.clauses) formula.push_back(c);
  
  decision_polarity.assign(f.variables,false);
  variable_activity.assign(f.variables, 0.);
  decision_order = set<variable, variable_cmp>
    (bind(variable_order_plugin,
          cref(*this),
          std::placeholders::_1,
          std::placeholders::_2));
  for (int i=0; i<f.variables; ++i) decision_order.insert(i);

  assignment.resize(f.variables,0);
  reasons.resize(f.variables*2);
  for (const auto& c : formula) working_clauses.push_back(c);
  restart();

  // Main loop
  solved = false;
  while(not solved) {
    conflicts.clear();
    assert(consistent());
    while(not propagation_queue.empty()) {
      unit_propagate();
      if (not conflicts.empty()) {
        learn();
        visualizer_plugin(assignment, working_clauses);
        if (solved) {
          LOG(LOG_EFFECTS) << "UNSAT" << endl;
          return proof(std::move(formula), std::move(learnt_clauses));
        }
      }
    }
    forget_plugin(*this);
    visualizer_plugin(assignment, working_clauses);
    decide();
    if (solved) {
      LOG(LOG_EFFECTS) << "This is a satisfying assignment:" << endl << assignment << endl;
      LOG(LOG_EFFECTS) << "SAT" << endl;
      exit(0);
    }
  }
  assert(false);
}



/*
 * Assignment management
 */

void cdcl::unit_propagate() {
  literal l = propagation_queue.front().to;
  if (propagation_queue.front().reason) {
    LOG(LOG_ACTIONS) << "Unit propagating " << l;
    LOG(LOG_ACTIONS) << " because of " << *propagation_queue.front().reason;
    LOG(LOG_ACTIONS) << endl;
  }
  else {
    LOG(LOG_DETAIL) << "Unit propagating " << l;
  }

  auto& al = assignment[variable(l)];
  if (propagation_queue.front().reason)
    reasons[l.l].push_back(propagation_queue.front().reason);
  if (al) {
    // A literal may be propagated more than once for different
    // reasons. We keep a list of them.
    assert(l.polarity()==(al==1));
    propagation_queue.pop();
    return;
  }

  branching_seq.push_back(propagation_queue.front());
  propagation_queue.pop();
  LOG(LOG_STATE_SUMMARY) << "Branching " << branching_seq << endl;

  assign(l);
}

void cdcl::assign(literal l) {
  auto& al = assignment[variable(l)];
  assert(not al);
  al = (l.polarity()?1:-1);

  // Restrict unit clauses and check for conflicts and new unit
  // propagations.
  std::set<literal>L;
  for (auto& c : working_clauses) {
    bool wasunit = c.unit();
    c.restrict(l);
    if (c.contradiction()) {
      if(!conflicts.empty()){puts("multiple conflicts");/*exit(1);*/}
      LOG(LOG_STATE) << "Conflict" << endl;
      conflicts.push_back(c.source);
    }
    if (c.unit() and not wasunit) {
      literal l = c.literals.front();
      if(L.count(l)){puts("multiple reasons");/*exit(1);*/}
      L.insert(l);
      LOG(LOG_STATE) << c << " is now unit" << endl;
      propagation_queue.propagate(c);
    }
  }
  
  decision_order.erase(variable(l));
  if (config_phase_saving) decision_polarity[variable(l)] = l.polarity();
}

void cdcl::unassign(literal l) {
  LOG(LOG_STATE) << "Backtracking " << l << endl;
  auto& al = assignment[variable(l)];
  assert(al);
  al = 0;

  for (auto& c : working_clauses) c.loosen(l);
  decision_order.insert(variable(l));
}



/*
 * Learning
 */

bool cdcl::asserting(const proof_clause& c) const {
  restricted_clause d(c);
  d.restrict(assignment);
  if (solved) return d.contradiction();
  return d.unit();
}

// 1UIP with all conflict graphs consistent with the current reasons.
proof_clause cdcl::learn_fuip_all(const branching_sequence::reverse_iterator& first_decision) {
  deque<clause> q;
  unordered_map<clause,pair<const clause*, const proof_clause*>> parent;
  for (auto& c : conflicts) {
    q.push_back(c->c);
    parent[c->c]={NULL,c};
  }
  assert(not q.empty());
  vector<clause> learnable_clauses;
  for (auto it = branching_seq.rbegin(); it!= first_decision; ++it) {
    literal l = it->to;
    deque<clause> qq;
    for (const auto& c:q) {
      if (not c.contains(~l)) {
        qq.push_back(c);
        continue;
      }
      for (auto d:reasons[l.l]) {
        clause e = resolve(c, d->c, variable(l));
        parent[e]={&(parent.find(c)->first), d};
        if (asserting(e)) learnable_clauses.push_back(e);
        else qq.push_back(e);
      }
    }
    swap(q,qq);
  }
  assert (not learnable_clauses.empty());
  int i=0;
  if (learnable_clauses.size()>1) {
    cout << "What do I learn? What do I learn?" << endl;
    cout << learnable_clauses << endl;
    cin >> i;
  }
  const clause& e = learnable_clauses[i];
  proof_clause ret(e);
  const clause* p = &e;
  while(p) {
    ret.derivation.push_back(parent[*p].second);
    p = parent[*p].first;
  }
  reverse(ret.derivation.begin(), ret.derivation.end());
  return ret;
}

// Find the learnt clause by resolving the conflict clause with
// reasons for propagation until the result becomes asserting (unit
// after restriction). The result is an input regular resolution
// proof.
proof_clause cdcl::learn_fuip(const branching_sequence::reverse_iterator& first_decision) {
  const proof_clause* conflict = conflicts.back();
  proof_clause c(conflict->c);
  c.derivation.push_back(conflict);
  for (auto it = branching_seq.rbegin(); it!=first_decision; ++it) {
    literal l = it->to;
    if (not c.c.contains(~l)) continue;
    if (asserting(c)) break;
    c.resolve(*it->reason, variable(l));
    LOG(LOG_DETAIL) << "Resolved with " << *reasons[l.l].front() << " and got " << c << endl;
  }
  return c;
}

// Find the learnt clause by resolving with all the reasons in the
// last decision level. Only the decision variable remains so the
// result must be asserting.
// aka: rel_sat
proof_clause cdcl::learn_luip(const branching_sequence::reverse_iterator& first_decision) {
  const proof_clause* conflict = conflicts.back();
  proof_clause c(conflict->c);
  c.derivation.push_back(conflict);
  for (auto it = branching_seq.rbegin(); it!=first_decision; ++it) {
    literal l = it->to;
    if (not c.c.contains(~l)) continue;
    c.resolve(*it->reason, variable(l));
    LOG(LOG_DETAIL) << "Resolved with " << *reasons[l.l].front() << " and got " << c << endl;
  }
  assert(asserting(c));
  return c;
}

// Learn a subset of the decision variables
proof_clause cdcl::learn_decision(const branching_sequence::reverse_iterator& first_decision) {
  const proof_clause* conflict = conflicts.back();
  proof_clause c(conflict->c);
  c.derivation.push_back(conflict);
  for (auto it = branching_seq.rbegin(); it!=branching_seq.rend(); ++it) {
    literal l = it->to;
    if (reasons[l.l].empty()) continue;
    if (not c.c.contains(~l)) continue;
    c.resolve(*it->reason, variable(l));
    LOG(LOG_DETAIL) << "Resolved with " << *reasons[l.l].front() << " and got " << c << endl;
  }
  assert(asserting(c));
  for (literal l : c.c) assert (reasons[l.l].empty());
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
  for (auto it=c.c.begin(); it!=c.c.end();) {
    literal l(*it);
    // We do not minimize asserting literals. We could be a bit bolder
    // here, but then we would require backjumps.
    if (l==asserting) {++it; continue;}
    for (const proof_clause* d:reasons[(~l).l]) {
      LOG(LOG_DETAIL) << "        Minimize? " << c.c << " vs " << *d << endl;
      if (d->c.subsumes(c.c, ~l)) {
        int i=it-c.begin();
        c.resolve(*d, variable(l));
        LOG(LOG_DETAIL) << Color::Modifier(Color::FG_GREEN) << "Minimize!" << Color::Modifier(Color::FG_DEFAULT) << endl;
        it=c.c.begin()+i;
        goto nextliteral;
      }
    }
    ++it;
  nextliteral:;
  }
}

void cdcl::backjump(const proof_clause& learnt_clause,
                    const branching_sequence::reverse_iterator& first_decision,
                    branching_sequence::reverse_iterator& backtrack_limit) {
  // We want to backtrack while the clause is unit
  for (; backtrack_limit != branching_seq.rend(); ++backtrack_limit) {
    bool found = false;
    for (auto l:learnt_clause) if (l.opposite(backtrack_limit->to)) found = true;
    if (found) break;
  }

  // But always backtrack to a decision
  while(backtrack_limit.base()->reason) --backtrack_limit;

  LOG(LOG_STATE_SUMMARY) << "Backjump: ";
  for (auto it=branching_seq.begin(); it!=branching_seq.end(); ++it) {
    if (it == backtrack_limit.base()) {
      LOG(LOG_STATE_SUMMARY) << "|   " << Color::Modifier(Color::TY_FAINT);
    }
    LOG(LOG_STATE_SUMMARY) << *it;
  }
  LOG(LOG_STATE_SUMMARY) << Color::Modifier(Color::DEFAULT) << endl;
  
  // Actually backtrack
  for (auto it=first_decision+1; it!=backtrack_limit; ++it) {
    unassign(it->to);
  }
}

void cdcl::learn() {
  LOG(LOG_STATE_SUMMARY) << "Conflict clause " << *conflicts.back() << endl;
  LOG(LOG_STATE) << "Assignment " << assignment << endl;
  LOG(LOG_STATE) << "Branching " << branching_seq << endl;
  // Backtrack to first decision level.
  auto first_decision = branching_seq.rbegin();
  for (;first_decision != branching_seq.rend(); ++first_decision) {
    unassign(first_decision->to);
    if (not first_decision->reason) break;
  }

  // If there is no decision on the stack, the formula is unsat.
  if (first_decision == branching_seq.rend()) solved = true;

  assert(first_decision != branching_seq.rbegin());
  learnt_clauses.emplace_back(learn_plugin(*this, first_decision));
  proof_clause& learnt_clause = learnt_clauses.back();

  if (config_minimize) minimize(learnt_clause);

  LOG(LOG_EFFECTS) << Color::Modifier(Color::FG_GREEN) << "Learnt: " << Color::Modifier(Color::FG_DEFAULT) << learnt_clause << endl;
  if(trace) *trace << "# learnt:" << learnt_clause << endl;
  clause d = learnt_clause.derivation.front()->c;
  for (auto it=++learnt_clause.derivation.begin();it!=learnt_clause.derivation.end();++it) {
    d = resolve(d,(*it)->c);
  }
  assert(not config_backjump or
         find_if(working_clauses.begin(), working_clauses.end(),
                 [&learnt_clause] (const restricted_clause& i) {
                   return i.source->c == learnt_clause.c;
                 }) == working_clauses.end());
  
  if (solved) {
    LOG(LOG_EFFECTS) << "I learned the following clauses:" << endl << learnt_clauses << endl;
    return;
  }

  // Complete backtracking
  auto backtrack_limit = first_decision;
  backtrack_limit++;
  if (config_backjump) backjump(learnt_clause, first_decision, backtrack_limit);
  for (auto it=backtrack_limit.base(); it!=branching_seq.end(); ++it) {
    reasons[it->to.l].clear();
  }
  branching_seq.erase(backtrack_limit.base(),branching_seq.end());

  bump_activity(learnt_clause.c);
  
  // Add the learnt clause to working clauses and immediately start
  // propagating
  working_clauses.push_back(learnt_clauses.back());
  working_clauses.back().restrict(assignment);
  assert(working_clauses.back().unit());
  LOG(LOG_STATE) << "Branching " << branching_seq << endl;
  // There may be a more efficient way to do this.
  propagation_queue.clear();
  for (const auto& c : working_clauses) {
    if (c.unit() and not assignment[variable(c.literals.front())]) {
      propagation_queue.propagate(c);
    }
  }

  conflicts.clear();
}



/*
 * Decision
 */

void cdcl::decide() {
  if (decision_order.empty()) {
    solved=true;
    return;
  }
  literal_or_restart decision = decide_plugin(*this);
  if (decision.restart) restart();
  else {
    LOG(LOG_DECISIONS) << "Deciding " << decision.l <<
      " with activity " << variable_activity[variable(decision.l)] << endl;
    if(trace) *trace << "assign " << pretty << variable(decision.l) << ' ' << decision.l.polarity() << endl;
    propagation_queue.decide(decision.l);
  }
}

literal_or_restart cdcl::decide_fixed() {
  int decision_variable = *decision_order.begin();
  decision_order.erase(decision_order.begin());
  return literal(decision_variable,decision_polarity[decision_variable]);
}

literal_or_restart cdcl::decide_askfile() {
  while(1) {
    std::string cmd; if(!(commands >> cmd)){puts("eof cmdfile");exit(1);}
    //cout << "cmd = " << cmd << "; ";
    if(cmd[0]=='d') {
      int l; if(!(commands >> l)){puts("eof cmdfile (2)");exit(1);}
      if(assignment[variable(from_dimacs(l))] != 0){cout<<"already assigned: "<<l<<endl;exit(1);}
      return from_dimacs(l);
    }
    else { // only for pebbling simulation.
      int node; if(!(commands >> node)){puts("eof cmdfile (3)");exit(1);}
      std::vector<variable>vv;vv.push_back(2*node-1),vv.push_back(2*node-2);
      forget_domain(vv);
    }
  }
}

void cdcl::restart() {
  LOG(LOG_ACTIONS) << "Restarting" << endl;
  for (auto branch:branching_seq) decision_order.insert(variable(branch.to));
  fill(assignment.begin(), assignment.end(), 0);
  fill(reasons.begin(), reasons.end(), list<const proof_clause*>());
  branching_seq.clear();
  propagation_queue.clear();

  for (auto& c : working_clauses) {
    c.reset();
    if (c.unit()) propagation_queue.propagate(c);
  }
}

void cdcl::bump_activity(const clause& c) {
  for (auto& x : variable_activity) x *= config_activity_decay;
  list<int> attach;
  for (auto l : c) {
    auto it = decision_order.find(variable(l));
    if (it != decision_order.end()) {
      attach.push_back(*it);
      decision_order.erase(it);
    }
    variable_activity[variable(l)] ++;
  }
  decision_order.insert(attach.begin(), attach.end());
}



/*
 * Forgetting
 */

void cdcl::forget_nothing() {}

// Forget clauses wider than w
void cdcl::forget_wide(int w) {
  assert(propagation_queue.empty());
  unordered_set<const proof_clause*> busy;
  for (auto branch : branching_seq) busy.insert(branch.reason);
  for (auto it = working_clauses.begin() + formula.size(); it!=working_clauses.end(); ) {
    if (it->source->c.width() > w and busy.count(it->source) == 0) {
      LOG(LOG_ACTIONS) << "Forgetting " << *it->source << endl;
      it = working_clauses.erase(it);
    }
    else {
      ++it;
    }
  }
}

void cdcl::forget_wide() {
  /*if (working_clauses.back().source->c.width() <= 2)*/ forget_wide(2);
}

void cdcl::forget_tseitin() {
  assert(propagation_queue.empty());
  unordered_set<const proof_clause*> busy;
  for (auto branch : branching_seq) busy.insert(branch.reason);
  for (auto it = working_clauses.begin() + formula.size(); it!=working_clauses.end(); ) {
    bool bad=!busy.count(it->source);
    if(it->source->c.width()==TSEITIN_H){
      bad=false;
      int what=-1;
      for(literal l:it->source->c)if(what==-1)what=l.l/2/TSEITIN_H;else if(l.l/2/TSEITIN_H!=what)bad=true;
    }
    if(bad){
      LOG(LOG_ACTIONS) << "Forgetting (T) " << *it->source << endl;
      it = working_clauses.erase(it);
    }
    else {
      ++it;
    }
  }
}

void cdcl::forget_domain(const vector<variable>& domain) {
  assert(propagation_queue.empty());
  vector<variable> dom(domain);
  sort(dom.begin(), dom.end());
  unordered_set<const proof_clause*> busy;
  for (auto branch : branching_seq) busy.insert(branch.reason);
  for (auto it = working_clauses.begin() + formula.size(); it!=working_clauses.end(); ) {
    if (includes(dom.begin(), dom.end(),
                 it->source->c.dom_begin(), it->source->c.dom_end())
        and busy.count(it->source) == 0) {
      LOG(LOG_ACTIONS) << "Forgetting " << *it->source << endl;
      it = working_clauses.erase(it);
    }
    else {
      ++it;
    }
  }
}

void cdcl::forget_everything() {
  assert(propagation_queue.empty());
  unordered_set<const proof_clause*> busy;
  for (auto branch : branching_seq) busy.insert(branch.reason);
  for (auto it = working_clauses.begin() + formula.size(); it!=working_clauses.end(); ) {
    if (busy.count(it->source) == 0) {
      LOG(LOG_ACTIONS) << "Forgetting " << *it->source << endl;
      it = working_clauses.erase(it);
    }
    else {
      ++it;
    }
  }
}

void cdcl::forget(unsigned int m) {
  assert(propagation_queue.empty());
  assert (m>=formula.size());
  assert (m<working_clauses.size());
  const auto& target = working_clauses[m];
  LOG(LOG_ACTIONS) << "Forgetting " << target << endl;
  for (const auto& branch : branching_seq) {
    if (branch.reason == target.source) {
      LOG(LOG_ACTIONS) << target << " is used to propagate " << branch.to << "; refusing to forget it." << endl;
      return;
    }
  }
  working_clauses.erase(working_clauses.begin()+m);
}

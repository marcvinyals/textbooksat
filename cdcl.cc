#include "cdcl.h"

#include <algorithm>
#include <iomanip>
#include <random>
#include <unordered_map>
#include <unordered_set>

#include <boost/function_output_iterator.hpp>

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
ostream& operator << (ostream& o, const proof_clause& c) {
  return o << (eager_restricted_clause(c));
}
ostream& operator << (ostream& o, const vector<clause>& v) {
  for (size_t i = 0; i<v.size(); ++i) o << setw(5) << i << ": " << v[i] << endl;
  return o;
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
  for (auto& c : working_clauses) {
    assert(not c.contradiction());
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
  variable_activity = initial_variable_activity(f);
  decision_order = set<variable, variable_cmp>
    (bind(variable_order_plugin,
          cref(*this),
          std::placeholders::_1,
          std::placeholders::_2));
  for (int i=0; i<f.variables; ++i) decision_order.insert(i);

  assignment.resize(f.variables,0);
  reasons.resize(f.variables*2);
  decision_level.resize(f.variables*2);
  working_clauses.set_variables(f.variables);
  for (const auto& c : formula) working_clauses.insert(c);
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
#ifndef NO_VIZ
        visualizer_plugin(assignment, working_clauses);
#endif
        if (solved) {
          LOG(LOG_EFFECTS) << "UNSAT" << endl;
          return proof(std::move(formula), std::move(learnt_clauses));
        }
      }
    }
    forget_plugin(*this);
#ifndef NO_VIZ
    visualizer_plugin(assignment, working_clauses);
#endif
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
  const branch& b = propagation_queue.front();
  literal l = b.to;
  if (b.reason) {
    LOG(LOG_ACTIONS) << "Unit propagating " << l;
    LOG(LOG_ACTIONS) << " because of " << *b.reason;
    LOG(LOG_ACTIONS) << endl;
  }
  else {
    LOG(LOG_DETAIL) << "Unit propagating " << l << endl;
  }

  auto& al = assignment[variable(l)];
  if (b.reason) reasons[l.l].push_back(b.reason);
  if (al) {
    // A literal may be propagated more than once for different
    // reasons. We keep a list of them.
    assert(l.polarity()==(al==1));
    propagation_queue.pop();
    return;
  }
  int last_decision_level = 0;
  if (not branching_seq.empty()) {
    last_decision_level = decision_level[branching_seq.back().to.l];
  }
  if (b.reason) decision_level[l.l] = last_decision_level + (b.reason?0:1);

  branching_seq.push_back(b);
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
  working_clauses.assign(l);
  
  decision_order.erase(variable(l));
  if (config_phase_saving) decision_polarity[variable(l)] = l.polarity();
}

void cdcl::unassign(literal l) {
  LOG(LOG_STATE) << "Backtracking " << l << endl;
  auto& al = assignment[variable(l)];
  assert(al);
  al = 0;

  working_clauses.unassign(l);

  decision_order.insert(variable(l));
}



/*
 * Learning
 */

bool cdcl::asserting(const proof_clause& c) const {
  eager_restricted_clause d(c);
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
  eager_restricted_clause d(c.c);
  d.restrict(assignment);
  if (d.contradiction()) return;
  literal asserting = d.literals.front();
  for (auto it=c.c.begin(); it!=c.c.end();) {
    literal l(*it);
    // We do not minimize asserting literals. We could be a bit bolder
    // here, but then we would require backjumps.
    if (l==asserting) {++it; continue;}
    for (const proof_clause* e:reasons[(~l).l]) {
      LOG(LOG_DETAIL) << "        Minimize? " << c.c << " vs " << *e << endl;
      if (e->c.subsumes(c.c, ~l)) {
        int i=it-c.begin();
        c.resolve(*e, variable(l));
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
  learnt_clause.trail = branching_seq;

  assert(not config_backjump or
         find_if(working_clauses.begin(), working_clauses.end(),
                 [&learnt_clause] (const auto& i) {
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
    decision_level[it->to.l]=-1;
  }
  branching_seq.erase(backtrack_limit.base(),branching_seq.end());

  bump_activity(learnt_clause);
  
  // Add the learnt clause to working clauses and immediately start
  // propagating
  working_clauses.insert(learnt_clauses.back(), assignment);

  LOG(LOG_STATE) << "Branching " << branching_seq << endl;
  // There may be a more efficient way to do this.
  propagation_queue.clear();
  for (const auto& c : working_clauses) {
    if (c.unit() and not assignment[variable(c.propagate().to)]) {
      propagation_queue.propagate(c);
    }
  }

  conflicts.clear();
}



/*
 * Activity
 */

unordered_set<variable> cdcl::bump_learnt(const proof_clause& c) {
  return unordered_set<variable>(c.c.dom_begin(), c.c.dom_end());
}

unordered_set<variable> cdcl::bump_conflict(const proof_clause& c) {
  unordered_set<variable> dom;
  for (const proof_clause* d : c.derivation) {
    dom.insert(d->c.dom_begin(), d->c.dom_end());
  }
  return dom;
}

void cdcl::bump_activity(const proof_clause& c) {
  for (auto& x : variable_activity) x *= config_activity_decay;
  vector<int> attach;
  for (auto v : bump_plugin(c)) {
    auto it = decision_order.find(v);
    if (it != decision_order.end()) {
      attach.push_back(*it);
      decision_order.erase(it);
    }
    variable_activity[v] ++;
  }
  decision_order.insert(attach.begin(), attach.end());
}

// Used in Minisat
vector<double> variable_activity_none(const cnf& f) {
  return vector<double>(f.variables,0);
}

vector<double> variable_activity_random(const cnf& f) {
  vector<double> variable_activity(f.variables);
  minstd_rand prg;
  for (double& x : variable_activity) {
    x = generate_canonical<double, 64>(prg);
  }
  return variable_activity;
}

// Used in Chaff
vector<double> variable_activity_weighted(const cnf& f) {
  vector<double> variable_activity(f.variables,0);
  for (const auto& c : f.clauses) {
    for (auto it = c.dom_begin(); it != c.dom_end(); ++it) {
      ++variable_activity[*it];
    }
  }
  return variable_activity;
}


vector<double> cdcl::initial_variable_activity(const cnf& f) {
  return variable_activity_none(f);
}



/*
 * Decision
 */

void cdcl::decide() {
  if (decision_order.empty()) {
    solved=true;
    return;
  }
  const literal_or_restart decision = decide_plugin(*this);
  if (decision.l == RESTART.l) restart();
  else {
    LOG(LOG_DECISIONS) << "Deciding " << decision.l <<
      " with activity " << variable_activity[variable(decision.l)] << endl;
    if(trace) *trace << "assign " << pretty << variable(decision.l) << ' ' << decision.l.polarity() << endl;
    propagation_queue.decide(decision.l);
  }
}

literal_or_restart cdcl::decide_random() {
  static minstd_rand prg;
  if (restart_plugin(*this)) return RESTART;
  auto it = decision_order.begin();
  advance(it, prg()%decision_order.size());
  int decision_variable = *it;
  decision_order.erase(it);
  return literal(decision_variable,decision_polarity[decision_variable]);
}

literal_or_restart cdcl::decide_fixed() {
  if (restart_plugin(*this)) return RESTART;
  int decision_variable = *decision_order.begin();
  decision_order.erase(decision_order.begin());
  return literal(decision_variable,decision_polarity[decision_variable]);
}

bool cdcl::restart_none() {
  return false;
}

bool cdcl::restart_fixed() {
  static uint last_conflict = 0;
  uint num_conflicts = learnt_clauses.size();
  if (num_conflicts > last_conflict) {
    last_conflict = num_conflicts;
    return true;
  }
  return false;
}

void cdcl::restart() {
  LOG(LOG_ACTIONS) << "Restarting" << endl;
  for (auto branch:branching_seq) decision_order.insert(variable(branch.to));
  fill(assignment.begin(), assignment.end(), 0);
  fill(reasons.begin(), reasons.end(), list<const proof_clause*>());
  fill(decision_level.begin(), decision_level.end(), -1);
  branching_seq.clear();
  propagation_queue.clear();

  working_clauses.reset();
}



/*
 * Forgetting
 */

void cdcl::forget_nothing() {}

// Forget clauses wider than w
void cdcl::forget_wide(unsigned int w) {
  forget_if([w](const clause& c) { return c.width() > w; });
}

void cdcl::forget_wide() {
  if (working_clauses.back().source->c.width() <= 2) forget_wide(2);
}

void cdcl::forget_if(const function<bool(clause)>& predicate) {
  assert(propagation_queue.empty());
  unordered_set<const proof_clause*> busy;
  for (auto branch : branching_seq) busy.insert(branch.reason);
  for (auto it = working_clauses.begin() + formula.size(); it!=working_clauses.end(); ) {
    if (predicate(it->source->c)
        and busy.count(it->source) == 0) {
      LOG(LOG_ACTIONS) << "Forgetting " << *it->source << endl;
      it = working_clauses.erase(it);
    }
    else {
      ++it;
    }
  }
  
}

void cdcl::forget_domain(const vector<variable>& domain) {
  vector<variable> dom(domain);
  sort(dom.begin(), dom.end());
  forget_if([&dom](const clause& c) {
      return includes(dom.begin(), dom.end(),
                      c.dom_begin(), c.dom_end());
    });
}

void cdcl::forget_touches_all(const vector<variable>& domain) {
  vector<variable> dom(domain);
  sort(dom.begin(), dom.end());
  forget_if([&dom](const clause& c) {
      return includes(c.dom_begin(), c.dom_end(),
                      dom.begin(), dom.end());
    });
}

void cdcl::forget_touches_any(const vector<variable>& domain) {
  vector<variable> dom(domain);
  sort(dom.begin(), dom.end());
  forget_if([&dom](const clause& c) {
      bool empty = true;
      auto set_is_empty = [&empty] (variable) { empty=false; };
      set_intersection(c.dom_begin(), c.dom_end(),
                       dom.begin(), dom.end(),
                       boost::make_function_output_iterator(set_is_empty));
      return not empty;
    });
    }

void cdcl::forget_everything() {
  forget_if([](const clause& c) { return true; });
}

void cdcl::forget(unsigned int m) {
  assert(propagation_queue.empty());
  assert (m>=formula.size());
  assert (m<working_clauses.size());
  auto it = working_clauses.begin()+m;
  const auto& target = *it;
  LOG(LOG_ACTIONS) << "Forgetting " << target << endl;
  for (const auto& branch : branching_seq) {
    if (branch.reason == target.source) {
      LOG(LOG_ACTIONS) << target << " is used to propagate " << branch.to << "; refusing to forget it." << endl;
      return;
    }
  }
  working_clauses.erase(it);
}

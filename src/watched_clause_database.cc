#include "watched_clause_database.h"

#include <iostream>

#include "formatting.h"
#include "colour.h"
#include "log.h"
#include "propagation_queue.h"

using namespace std;

ostream& operator << (ostream& o, const watched_clause& c) {
  if (c.satisfied) o << Colour::Modifier(Colour::TY_CROSSED);
  for (auto l:c.literals) o << l;
  if (c.satisfied) o << Colour::Modifier(Colour::DEFAULT);
  return o;
}

constexpr literal NONE = literal::from_raw(-1);

literal watched_clause::restrict(literal l,const std::vector<int>& assignment) {
  if (satisfied) return NONE;
  if (literals[0]==~l) {
    literal new_watch = find_new_watch(0, assignment);
    if (new_watch == NONE) {
      if (--unassigned==1) {
        swap(literals[0],literals[1]);
      }
    }
    return new_watch;
  }
  assert (literals[1]==~l);
  {
    literal new_watch = find_new_watch(1, assignment);
    if (new_watch == NONE) {
      --unassigned;
    }
    return new_watch;
  }
  assert(false);
}

void watched_clause::satisfy() {
  assert(unassigned);
  satisfied++;
}

void watched_clause::loosen_falsified(literal l) {
  if(unassigned<2) ++unassigned;
  literals_visible_size=literals.size();
}

void watched_clause::loosen_satisfied(literal l) {
  assert(unassigned);
  satisfied--;
  literals_visible_size=literals.size();
}

void watched_clause::restrict_to_unit(const std::vector<int>& assignment,
                                      const std::vector<int>& decision_level) {
  auto other_watch=literals.begin();
  for (auto it=literals.begin();it!=literals.end();++it) {
    int al = assignment[variable(*it)];
    if (not al) {
      swap(literals[0],*it);
      if (other_watch==literals.begin()) other_watch=it;
    }
    else {
      assert((al==1)!=it->polarity());
      if (decision_level[(~*it).l]>decision_level[(~*other_watch).l]) {
        other_watch=it;
      }
      --literals_visible_size;
    }
  }
  if (literals.size()>1) swap(literals[1],*other_watch);
  assert(literals_visible_size==1);
  unassigned=1;
}

literal watched_clause::find_new_watch(size_t replaces, const std::vector<int>& assignment) {
  int iters=0;
  while(literals_visible_size>2) {
    assert(iters<=2*literals.size());
    ++iters;
    literal& l = literals[literals_visible_size-1];
    int al = assignment[variable(l)];
    if (al) {
      if ((al==1)==l.polarity()) {
        swap(literals[replaces],l);
        satisfy();
        return literals[replaces];
      }
      else {
        --literals_visible_size;
      }
    }
    else {
      swap(literals[replaces],l);
      --literals_visible_size;
      return literals[replaces];
    }
  }
  return NONE;
}

void watched_clause::reset() {
  literals_visible_size = literals.size();
  unassigned = min(2, int(literals_visible_size));
  satisfied = 0;
}

void watched_clause_database::assign(literal l) {
  for (size_t i : watches[l.l]) {
    watched_clause& c = working_clauses[i];
    c.satisfy();
  }
  auto& this_watch = watches[(~l).l];
  for (auto it = this_watch.begin(); it!=this_watch.end(); ++it) {
    watched_clause& c = working_clauses[*it];
    literal new_watch = c.restrict(l, assignment);
    if (c.satisfied) {
      if (not (new_watch == NONE)) {
        watches[new_watch.l].push_back(*it);
      }
    }
    else if (new_watch == NONE) {
      if (c.contradiction()) {
        LOG(LOG_STATE) << "Conflict" << endl;
        this->conflicts.push_back(c.source);
      }
      if (c.unit()) {
        c.assert_unit(assignment);
        LOG(LOG_STATE) << c << " is now unit" << endl;
        this->propagation_queue.propagate(c);
      }
    }
    else {
      watches[new_watch.l].push_back(*it);
    }
  }
}

void watched_clause_database::unassign(literal l) {
  for (size_t i : watches[l.l]) {
    watched_clause& c = working_clauses[i];
    c.loosen_satisfied(l);
  }
  auto& this_watch = watches[(~l).l];
  size_t this_watch_size=this_watch.size();
  for (size_t it = 0; it<this_watch_size;) {
    size_t i = this_watch[it];
    watched_clause& c = working_clauses[i];
    c.loosen_falsified(l);
    if (c.literals[0]==~l or (c.unassigned==2 and c.literals[1]==~l)) {
      ++it;
    }
    else {
      swap(this_watch[it],this_watch[this_watch_size-1]);
      --this_watch_size;
    }
  }
  this_watch.resize(this_watch_size);
}

void watched_clause_database::reset() {
  for (auto& w : watches) w.clear();
  for (size_t i=0; i< working_clauses.size(); ++i) {
    watched_clause& c = working_clauses[i];
    c.reset();
    if(c.literals.size()>=1) watches[c.literals[0].l].push_back(i);
    if(c.literals.size()>=2) watches[c.literals[1].l].push_back(i);
    if (c.unit()) propagation_queue.propagate(c);
  }
}

void watched_clause_database::insert(const proof_clause& c, const std::vector<int>& assignment) {
  working_clauses.push_back(c);
  watched_clause& cc = working_clauses.back();
  cc.restrict_to_unit(assignment, decision_level);
  cc.assert_unit(assignment);

  size_t i = working_clauses.size()-1;
  for (literal l : cc.literals) {
    watches[l.l].push_back(i);
  }
}

void watched_clause_database::insert(const proof_clause& c) {
  working_clauses.push_back(c);
}

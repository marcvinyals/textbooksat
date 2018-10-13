#include "clause_database.h"

#include <iostream>

#include "formatting.h"
#include "color.h"
#include "log.h"
#include "propagation_queue.h"

using namespace std;

ostream& operator << (ostream& o, const eager_restricted_clause& c) {
  if (c.satisfied) o << Color::Modifier(Color::TY_CROSSED);
  for (auto l:c.literals) o << l;
  if (c.satisfied) o << Color::Modifier(Color::DEFAULT);
  return o;
}
ostream& operator << (ostream& o, const lazy_restricted_clause& c) {
  if (c.satisfied) o << Color::Modifier(Color::TY_CROSSED);
  for (auto it = c.source->begin(); it!=c.source->end(); ++it) {
    if (c.literals[it-c.source->begin()]) o << *it;
  }
  if (c.satisfied) o << Color::Modifier(Color::DEFAULT);
  return o;
}
ostream& operator << (ostream& o, const watched_clause& c) {
  //for (auto l:*c.source) o << l;

  //if (c.satisfied) o << Color::Modifier(Color::TY_CROSSED);
  for (auto l:c.literals) o << l;
  //if (c.satisfied) o << Color::Modifier(Color::DEFAULT);
  return o;
}

void eager_restricted_clause::restrict(literal l) {
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

void eager_restricted_clause::loosen(literal l) {
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

void eager_restricted_clause::restrict(const std::vector<int>& assignment) {
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

void eager_restricted_clause::reset() {
  literals.assign(source->begin(), source->end());
  satisfied = 0;
}

void lazy_restricted_clause::restrict(literal l) {
  if (satisfied) return;
  literal labs = l.abs();
  auto it = lower_bound(source->begin(), source->end(), labs);
  if (it==source->end()) return;
  if (*it==l) {
    satisfied=true;
    satisfied_literal=l;
  }
  else if (it->opposite(l)) {
    unassigned--;
    literals.reset(it-source->begin());
  }
}

void lazy_restricted_clause::loosen(literal l) {
  if (satisfied) {
    if (l==satisfied_literal) satisfied=false;
  }
  else {
    literal nl = ~l;
    auto it = lower_bound(source->begin(), source->end(), nl);
    if (it==source->end()) return;
    if (*it==nl) {
      unassigned++;
      literals.set(it-source->begin());
    }
  }
}

void lazy_restricted_clause::restrict_to_unit(const std::vector<int>& assignment) {
  for (auto it=source->begin();it!=source->end();++it) {
    int al = assignment[variable(*it)];
    if (al) {
      assert((al==1)!=it->polarity());
      unassigned--;
      literals.reset(it-source->begin());
    }
  }
}

branch lazy_restricted_clause::propagate() const {
  return {*(source->begin()+literals.find_first()),source};
}

void lazy_restricted_clause::reset() {
  satisfied = false;
  unassigned = source->c.width();
  literals.set();
}

template<typename T>
void reference_clause_database<T>::assign(literal l) {
  for (auto& c : this->working_clauses) {
    bool wasunit = c.unit();
    c.restrict(l);
    if (c.contradiction()) {
      LOG(LOG_STATE) << "Conflict" << endl;
      this->conflicts.push_back(c.source);
    }
    if (c.unit() and not wasunit) {
      LOG(LOG_STATE) << c << " is now unit" << endl;
      this->propagation_queue.propagate(c);
    }
  }
}

template<typename T>
void reference_clause_database<T>::unassign(literal l) {
  for (auto& c : this->working_clauses) c.loosen(l);
}

template<typename T>
void reference_clause_database<T>::reset() {
  for (auto& c : this->working_clauses) {
    c.reset();
    if (c.unit()) this->propagation_queue.propagate(c);
  }
}

template class reference_clause_database<eager_restricted_clause>;
template class reference_clause_database<lazy_restricted_clause>;








constexpr literal NONE = literal::from_raw(-1);

literal watched_clause::restrict(literal l, const std::vector<int>& assignment) {
  assert(not satisfied);
  //cerr << unassigned << endl;
  if (literals[0]==~l) {
    //cerr << "Lit 0 dies" << endl;
    literal new_watch = find_new_watch(0, assignment);
    //cerr << "New watch " << new_watch << endl;
    if (new_watch == NONE) {
      if (--unassigned==1) {
        swap(literals[0],literals[1]);
      }
    }
    //cerr << unassigned << endl;
    return new_watch;
  }
  assert (literals[1]==~l);
  {
    //cerr << "Lit 1 dies" << endl;
    literal new_watch = find_new_watch(1, assignment);
    //cerr << "New watch " << new_watch << endl;
    if (new_watch == NONE) {
      --unassigned;
    }
    //cerr << unassigned << endl;
    return new_watch;
  }
  assert(false);
}

literal watched_clause::satisfy(literal l) {
  assert(unassigned);
  satisfied=true;
  if (literals[0]==l) {
    if (unassigned>=2) return literals[1];
    return NONE;
  }
  assert(literals[1]==l);
  return literals[0];
}

void watched_clause::loosen_falsified(literal l) {
  ++unassigned;
  literals_visible_size=literals.size();
}

literal watched_clause::loosen_satisfied(literal l) {
  assert(unassigned);
  satisfied=false;
  literals_visible_size=literals.size();
  if (literals[0]==l) {
    if (unassigned>=2) return literals[1];
    return NONE;
  }
  assert(literals[1]==l);
  return literals[0];
}

void watched_clause::restrict_to_unit(const std::vector<int>& assignment) {
  for (auto it=literals.begin();it!=literals.end();++it) {
    int al = assignment[variable(*it)];
    if (not al) {
      swap(literals[0],*it);
    }
    else {
      assert((al==1)!=it->polarity());
      --literals_visible_size;
    }
  }
  cerr << "Oy this is rt1 have " << literals_visible_size << endl;
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
        satisfied=true;
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
  //cerr << "Reset?!" << endl;
  literals_visible_size = literals.size();
  unassigned = min(2, int(literals_visible_size));
  satisfied = false;
}

void watched_clause_database::assign(literal l) {
  /*for (size_t i=0; i<watches.size(); ++i) {
    const auto& w = watches[i];
    cerr << literal::from_raw(i) << " watches ";
    for (size_t j : w) cerr << working_clauses[j] << "   ";
    cerr << endl;
  }*/
  
  for (size_t i : watches[l.l]) {
    watched_clause& c = working_clauses[i];
    if (i==79046) cerr << "Satisfying " << c << endl;
    literal stop_watch = c.satisfy(l);
    if (not (stop_watch == NONE)) {
      watches[stop_watch.l].erase(i);
    }
  }
  auto& this_watch = watches[(~l).l];
  for (auto it = this_watch.begin(); it!=this_watch.end();) {
    watched_clause& c = working_clauses[*it];

    if (*it==79046) cerr << "Hitting " << c << endl;
    
    if (c.satisfied) {
      cerr << "Fail incoming " << *it << endl << c << endl;
    }

    literal new_watch = c.restrict(l, assignment);
    if (c.satisfied) {
      assert (not (new_watch == NONE));
      cerr << "Satisfying " << c << endl;
      literal stop_watch = c.satisfy(new_watch);
      assert(not (stop_watch == NONE));
      watches[stop_watch.l].erase(*it);
      watches[new_watch.l].insert(*it);
      it=this_watch.erase(it);
    }
    else if (new_watch == NONE) {
      if (c.contradiction()) {
        LOG(LOG_STATE) << "Conflict" << endl;
        this->conflicts.push_back(c.source);
      }
      if (c.unit()) {
        LOG(LOG_STATE) << c << " is now unit" << endl;
        this->propagation_queue.propagate(c);
      }
      ++it;
    }
    else {
      watches[new_watch.l].insert(*it);
      it=this_watch.erase(it);
    }
  }
}

void watched_clause_database::unassign(literal l) {
  //cerr << "Unwatching " << l << endl;
  for (size_t i : watches[l.l]) {
    watched_clause& c = working_clauses[i];
    literal new_watch = c.loosen_satisfied(l);
    if (not (new_watch == NONE)) {
      watches[new_watch.l].insert(i);
    }
  }
  for (size_t i : watches[(~l).l]) working_clauses[i].loosen_falsified(l);
}

void watched_clause_database::reset() {
  for (auto& w : watches) w.clear();
  for (size_t i=0; i< working_clauses.size(); ++i) {
    watched_clause& c = working_clauses[i];
    c.reset();
    if(c.literals.size()>=1) watches[c.literals[0].l].insert(i);
    if(c.literals.size()>=2) watches[c.literals[1].l].insert(i);
    if (c.unit()) propagation_queue.propagate(c);
  }
}

void watched_clause_database::insert(const proof_clause& c, const std::vector<int>& assignment) {
  working_clauses.push_back(c);
  watched_clause& cc = working_clauses.back();
  cc.restrict_to_unit(assignment);
  assert(cc.unit());
  //cerr << "Asserting watch " << cc.literals[0].l << endl;
  watches[cc.literals[0].l].insert(working_clauses.size()-1);
}

void watched_clause_database::insert(const proof_clause& c) {
  working_clauses.push_back(c);
}

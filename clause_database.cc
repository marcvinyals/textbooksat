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

const size_t BUGGY = -1;//133;// 81; //136217; //12583;

literal watched_clause::restrict(literal l, const std::vector<int>& assignment) {
  if(satisfied) return NONE;
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
  //cerr << *this << endl;
  auto other_watch=literals.begin();
  //cerr << "Maybe " << *other_watch << endl;
  for (auto it=literals.begin();it!=literals.end();++it) {
    int al = assignment[variable(*it)];
    if (not al) {
      swap(literals[0],*it);
      if (other_watch==literals.begin()) other_watch=it;
    }
    else {
      assert((al==1)!=it->polarity());
      if (decision_level[(~*it).l]>decision_level[(~*other_watch).l]) {
        //cerr << "Rather pick " << *it << " than " << *other_watch << endl;
        other_watch=it;
      }
      --literals_visible_size;
    }
  }
  if (literals.size()>1) swap(literals[1],*other_watch);
  assert(literals_visible_size==1);
  unassigned=1;
  //cerr << *this << endl;
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
  //cerr << "Reset?!" << endl;
  literals_visible_size = literals.size();
  unassigned = min(2, int(literals_visible_size));
  satisfied = 0;
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
    if (i==BUGGY) cerr << "Satisfying " << c << endl;
    c.satisfy();
  }
  auto& this_watch = watches[(~l).l];
  for (auto it = this_watch.begin(); it!=this_watch.end(); ++it) {
    watched_clause& c = working_clauses[*it];

    if (*it==BUGGY) {
      cerr << "Hitting " << c << " with " << l << endl;
      cerr << "  have " << c.literals_visible_size << " visibles" << endl;
    }

    if (not (c.literals[0]==~l) and not (c.literals[1]==~l)) {
      cerr << "Fail incoming " << *it << endl << c << endl;
    }

    literal new_watch = c.restrict(l, assignment);

    if (*it==BUGGY) {
      cerr << "Now it is " << c << " watched by " << new_watch << endl;
      cerr << "  have " << c.literals_visible_size << " visibles" << endl;
    }

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
#ifdef DEBUG
        eager_restricted_clause cc(*c.source);
        cc.restrict(assignment);
        if (not cc.unit()) {
          cerr << "Fail incoming " << *it << endl << c << endl;
        }
        assert(cc.unit());
#endif
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
  //cerr << "Unwatching " << l << endl;
  for (size_t i : watches[l.l]) {
    watched_clause& c = working_clauses[i];
    if (i==BUGGY) cerr << "Sat-Loosening " << c << " with " << l << endl;
    c.loosen_satisfied(l);
  }
  auto& this_watch = watches[(~l).l];
  size_t this_watch_size=this_watch.size();
  for (size_t it = 0; it<this_watch_size;) {
    size_t i = this_watch[it];
    watched_clause& c = working_clauses[i];
    if (i==BUGGY) cerr << "Loosening " << c << " with " << l << endl;
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
  assert(cc.unit());
#ifdef DEBUG
  eager_restricted_clause ccc(*cc.source);
  ccc.restrict(assignment);
  assert(ccc.unit());
#endif
  //cerr << "Asserting watch " << cc.literals[0].l << endl;
  size_t i = working_clauses.size()-1;
  if (i==BUGGY) max_log_level = LOG_STATE_SUMMARY;
  for (literal l : cc.literals) {
    watches[l.l].push_back(i);
  }
}

void watched_clause_database::insert(const proof_clause& c) {
  working_clauses.push_back(c);
}

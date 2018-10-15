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

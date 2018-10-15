#pragma once

#include <deque>

#include "clause_database.h"

struct propagation_queue {
  std::deque<branch> q;
  template<typename T>
  void propagate(const T& c) {
    assert(c.unit());
    q.push_back(c.propagate());
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

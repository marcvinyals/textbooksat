#pragma once

#include <deque>

#include "data_structures.h"
#include "pebble_util.h"

class cdcl;
struct literal_or_restart;

class pebble {
 public:
  pebble(std::istream& graph, const std::string& fn, int arity);
  literal_or_restart get_decision(cdcl& solver);

 private:
  void pebble_next(cdcl& solver);
  
  std::vector<std::vector<int>> g;
  substitution sub;
  std::deque<int> pebble_queue;
  std::vector<int> expect;
  std::deque<literal> decision_queue;
};

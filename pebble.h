#pragma once

#include <deque>
#include <istream>
#include <list>
#include <utility>

#include "data_structures.h"
#include "pebble_util.h"

class cdcl;
struct literal_or_restart;

class pebble {
 public:
  pebble(std::istream& graph, const std::string& fn, int arity);
  void bindto(cdcl& solver);
  literal_or_restart get_decision();

 private:
  void pebble_next2();
  bool is_complete(int u) const;
  void prepare_successors();
  void cleanup();
  void update_truth();
  std::vector<int> parse_sequence(std::istream& in);
  
  std::vector<std::vector<int>> g;
  std::vector<std::vector<int>> rg;
  substitution sub;
  std::deque<std::pair<int,int>> pebble_queue;
  std::vector<int> expect;
  std::deque<literal> decision_queue;
  cdcl* solver;
  std::vector<int> truth;
};

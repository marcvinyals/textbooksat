#pragma once

#include <deque>
#include <istream>

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
  void pebble_next();
  void sequence_next();
  void prepare_successors(int who);
  void prepare_successors();
  void cleanup();
  void parse_sequence(std::istream& in);
  
  std::vector<std::vector<int>> g;
  std::vector<std::vector<int>> rg;
  substitution sub;
  std::deque<int> pebble_sequence;
  std::deque<int> pebble_queue;
  std::deque<int> something_else;
  bool skip;
  std::vector<int> expect;
  std::deque<literal> decision_queue;
  cdcl* solver;
  std::vector<int> truth;
};

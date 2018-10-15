#pragma once

#include <string>
#include <vector>

#include "cdcl.h"

class ui {
 public:
  ui(cdcl& from_solver) : solver(from_solver), batch(false) {}
  ui(const ui&) = delete;
  ui& operator = (const ui&) = delete;

  bool get_restart();
  literal get_decision();

 private:
  void usage();
  void show_state();

  enum action {
    ASK=0,
    DECISION,
    RESTART,
  };
  struct answer {
    enum action action;
    literal decision;
    answer(literal l) : action(DECISION), decision(l) {}
    answer(enum action action) : action(action), decision(literal::from_raw(-1)) {}
    answer() : answer(ASK) {}
  };
  answer ask();
  answer next_answer;

  cdcl& solver;
  std::vector<std::string> history;
  bool batch;
};

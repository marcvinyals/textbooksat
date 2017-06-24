#pragma once

#include <string>
#include <vector>

#include "cdcl.h"

class ui {
 public:
  ui(cdcl& solver) : solver(solver), batch(false) {}
  literal_or_restart get_decision();

 private:
  void usage();
  void show_state();

  cdcl& solver;
  std::vector<std::string> history;
  bool batch;
};

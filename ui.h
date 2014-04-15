#pragma once

#include <string>
#include <vector>

class cdcl;
struct literal_or_restart;

class ui {
 public:
  ui(cdcl& solver) : solver(solver), batch(false) {}
  literal_or_restart get_decision();
 private:
  cdcl& solver;
  std::vector<std::string> history;
  bool batch;
};

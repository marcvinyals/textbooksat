#pragma once

#include <string>
#include <vector>

class cdcl;
struct literal_or_restart;

class ui {
 public:
  static void bindto(cdcl& solver);
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

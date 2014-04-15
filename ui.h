#pragma once

#include <string>
#include <vector>

class cdcl;
struct literal_or_restart;

struct ui {
  cdcl& solver;
  std::vector<std::string> history;
  literal_or_restart get_decision();
};

#pragma once

#include "data_structures.h"

#include <string>

struct cdcl_solver {
 public:
  proof solve(const cnf& f);
  std::string decide, learn;
  bool backjump, minimize;
};

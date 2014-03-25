#pragma once

#include "data_structures.h"

#include <string>
#include <functional>

struct cdcl_solver {
 public:
  void solve(const cnf& f);
  std::string decide, learn;
};

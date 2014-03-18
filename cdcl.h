#pragma once

#include "data_structures.h"

#include <string>
#include <functional>

struct cdcl_solver {
 public:
  cdcl_solver() : decide("fixed") {}
  void solve(const cnf& f);
  std::string decide;
};

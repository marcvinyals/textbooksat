#pragma once

#include "data_structures.h"

#include <string>
#include <ostream>
#include <memory>

struct cdcl_solver {
 public:
  proof solve(const cnf& f);
  std::string decide, learn, forget;
  bool backjump, minimize, phase_saving;
  std::shared_ptr<std::ostream> trace;
  std::shared_ptr<std::istream> pebbling;
};

#pragma once

#include <string>
#include <ostream>
#include <memory>

#include "data_structures.h"
#include "viz.h"

struct cdcl_solver {
 public:
  proof solve(const cnf& f);
  std::string decide, restart, learn, forget, bump, watcher, phase;
  double decay, clause_decay;
  bool backjump, minimize;
  std::shared_ptr<std::ostream> trace;
  std::shared_ptr<graphviz_viz> vz;
};

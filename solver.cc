#include "solver.h"

#include "cdcl.h"
#include "formatting.h"

using namespace std;

proof cdcl_solver::solve(const cnf& f) {
  pretty = pretty_(f);
  static cdcl solver;
  if (decide == "ask") {
    solver.decide_plugin = &cdcl::decide_ask;
    solver.variable_order_plugin = &cdcl::variable_cmp_fixed;
  }
  else {
    solver.decide_plugin = &cdcl::decide_fixed;
    if (decide == "fixed")
      solver.variable_order_plugin = &cdcl::variable_cmp_fixed;
    else if (decide == "reverse")
      solver.variable_order_plugin = &cdcl::variable_cmp_reverse;
    else if (decide == "vsids")
      solver.variable_order_plugin = &cdcl::variable_cmp_vsids;
    else {
      cerr << "Invalid decision procedure" << endl;
      exit(1);
    }
  }
  if (learn == "1uip") solver.learn_plugin = &cdcl::learn_fuip;
  else if (learn == "lastuip") solver.learn_plugin = &cdcl::learn_luip;
  else if (learn == "1uip-all") solver.learn_plugin = &cdcl::learn_fuip_all;
  else if (learn == "decision") solver.learn_plugin = &cdcl::learn_decision;
  else {
    cerr << "Invalid learning scheme" << endl;
    exit(1);
  }
  solver.config_backjump = backjump;
  solver.config_minimize = minimize;
  solver.config_phase_saving = phase_saving;
  solver.config_activity_decay = 1.-1./32.;
  return solver.solve(f);
}

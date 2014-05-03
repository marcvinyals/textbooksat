#include "solver.h"

#include "cdcl.h"
#include "formatting.h"
#include "pebble.h"
#include "ui.h"
#ifndef NO_VIZ
#include "viz.h"
#endif

using namespace std;

void visualizer_nothing(const vector<int>&, const vector<restricted_clause>&) {}

proof cdcl_solver::solve(const cnf& f) {
  pretty = pretty_(f);
  pretty.mode = pretty.TERMINAL;
  cdcl solver;
  if (decide == "ask") {
    ui::bindto(solver);
    solver.variable_order_plugin = &cdcl::variable_cmp_fixed;
  }
  else if (decide == "pebble") {
    solver.decide_plugin = bind(&pebble::get_decision, pebble_helper, std::placeholders::_1);
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
  if (forget == "nothing") solver.forget_plugin = &cdcl::forget_nothing;
  else if (forget == "everything") solver.forget_plugin = &cdcl::forget_everything;
  else if (forget == "wide") solver.forget_plugin = static_cast<void (cdcl::*)(void)>(&cdcl::forget_wide);
  else {
    cerr << "Invalid forgetting scheme" << endl;
    exit(1);
  }
#ifndef NO_VIZ
  if (vz) {
    solver.visualizer_plugin =
      bind(&pebble_viz::draw, ref(*vz),
           std::placeholders::_1, std::placeholders::_2);
  }
  else {
#else
  {
#endif
    solver.visualizer_plugin = &visualizer_nothing;
  }
  solver.config_backjump = backjump;
  solver.config_minimize = minimize;
  solver.config_phase_saving = phase_saving;
  solver.config_activity_decay = 1.-1./32.;
  solver.trace = trace;
  return solver.solve(f);
}

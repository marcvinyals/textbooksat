#include "solver.h"

#include "cdcl.h"
#include "formatting.h"
#include "ui.h"
#ifndef NO_VIZ
#include "vizpebble.h"
#endif

using namespace std;

#ifndef NO_VIZ
void visualizer_nothing(const vector<int>&, const vector<restricted_clause>&) {}
#endif

cdcl solver_factory(const string& watcher) {
  if (watcher == "reference") {
    return (reference_clause_database<eager_restricted_clause>*)nullptr;
  }
  else if (watcher == "2wl") {
    return cdcl((watched_clause_database*)nullptr);
  }
  else {
    cerr << "Invalid clause watcher" << endl;
    exit(1);
  }
}

proof cdcl_solver::solve(const cnf& f) {
  if (decide == "ask" and watcher == "2wl") {
    cerr << "Warning: 2wl in interactive mode. Printing the state will crash." << endl;
  }
  pretty = pretty_(f);
  pretty.mode = pretty.TERMINAL;
  cdcl solver(solver_factory(watcher));
  class ui ui (solver);
  if (decide == "ask") {
    solver.decide_plugin = bind(&ui::get_decision, ref(ui));
    solver.variable_order_plugin = &cdcl::variable_cmp_fixed;
  }
  else if (decide == "random") {
    solver.decide_plugin = &cdcl::decide_random;
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
  if (decide == "ask") solver.restart_plugin = bind(&ui::get_restart, ref(ui));
  else if (restart == "none") solver.restart_plugin = &cdcl::restart_none;
  else if (restart == "always") solver.restart_plugin = &cdcl::restart_always;
  else if (restart == "luby") solver.restart_plugin = &cdcl::restart_luby;
  else {
    cerr << "Invalid restart interval" << endl;
    exit(1);
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
      bind(&graphviz_viz::draw, ref(*vz),
           std::placeholders::_1, std::placeholders::_2);
  }
  else {
    solver.visualizer_plugin = &visualizer_nothing;
  }
#endif
  if (bump == "learnt") solver.bump_plugin = &cdcl::bump_learnt;
  else if (bump == "conflict") solver.bump_plugin = &cdcl::bump_conflict;
  else {
    cerr << "Invalid bump plugin" << endl;
    exit(1);
  }
  solver.config_backjump = backjump;
  solver.config_minimize = minimize;
  if (phase == "save") {
    solver.config_default_polarity = false;
    solver.config_phase_saving = true;
  }
  else {
    solver.config_phase_saving = false;
    solver.config_default_polarity = atoi(phase.c_str());
  }
  solver.config_activity_decay = decay;
  solver.config_clause_decay = clause_decay;
  solver.trace = trace;
  return solver.solve(f);
}

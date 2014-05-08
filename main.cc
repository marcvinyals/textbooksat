#include <iostream>
#include <fstream>
#include <argp.h>

#include "dimacs.h"
#include "solver.h"
#include "analysis.h"
#include "log.h"
#include "pebble.h"
#ifndef NO_VIZ
#include "viz.h"
#endif

using namespace std;

static argp_option options[] = {
  {"in", 'i', "FILE", 0,
   "Read formula in dimacs format from FILE (default: stdin)"},
  {"decide", 'd', "{fixed,reverse,vsids,ask}", 0,
   "Use the specified decision procedure (default: fixed)"},
  {"learn", 'l', "{1uip,1uip-all,lastuip,decision}", 0,
   "Use the specified learning schema (default: 1uip)"},
  {"forget", 'f', "{nothing,everything,wide}", 0,
   "Use the specified forgetting schema (default: nothing)"},
  {"backjump", 'b', "BOOL", 0,
   "On a conflict, backtrack deeper than the decision level as long as the "
   "learnt clause is unit (default: 1)\nNote that disabling backjumps may "
   "result in learning the same clause repeatedly if the learnt clause "
   "does not have any literal in the second-to-last decision "
   "level. Therefore it is recommended to leave this option alone."},
  {"minimize", 'm', "BOOL", 0,
   "Try to subsume the learnt clause by resolving it with some other "
   "clause in the database (default: 0)"},
  {"phase-saving", 's', "BOOL", 0,
   "When deciding a variable, set it to the polarity it was last assigned "
   "to. (default: 1)"},
  {"proof-dag", 'p', "FILE", 0,
   "Output the proof dag to FILE (default: null)"},
  {"trace", 't', "FILE", 0,
   "Output the decision sequence to FILE (default: null)"},
  {"pebbling-graph", 1, "FILE", 0,
   "Visualize pebbling from FILE (default: null)"},
  {"substitution-fn", 2, "{xor}", 0,
   "Hint that the formula was substituted with the specified function (default: xor)"},
  {"substitution-arity", 3, "INT", 0,
   "Hint that the formula was substituted with the specified arity (default: 2)"},
  {"viz",4,"BOOL",0,""},
  {"verbose", 'v', "[0..3]", 0,
   "Verbosity level"},
  { 0 }
};

struct arguments {
  string in;
  string decide;
  string learn;
  string forget;
  bool backjump;
  bool minimize;
  bool phase_saving;
  string dag;
  string trace;
  string pebbling_graph;
  string substitution_fn;
  int substitution_arity;
  bool viz;
  int verbose;
};

static error_t parse_opt (int key, char *arg, argp_state *state) {
  arguments* arguments = (struct arguments*)(state->input);
  switch(key) {
  case 'i':
    arguments->in = arg;
    break;
  case 'd':
    arguments->decide = arg;
    break;
  case 'l':
    arguments->learn = arg;
    break;
  case 'f':
    arguments->forget = arg;
    break;
  case 'b':
    arguments->backjump = atoi(arg);
    break;
  case 'm':
    arguments->minimize = atoi(arg);
    break;
  case 's':
    arguments->phase_saving = atoi(arg);
    break;
  case 'p':
    arguments->dag = arg;
    break;
  case 't':
    arguments->trace = arg;
    break;
  case 1:
    arguments->pebbling_graph = arg;
    break;
  case 2:
    arguments->substitution_fn = arg;
    break;
  case 3:
    arguments->substitution_arity = atoi(arg);
    break;
  case 4:
    arguments->viz = atoi(arg);
    break;
  case 'v':
    arguments->verbose = atoi(arg);
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, 0, 0 };

bool endswith(string s, string t) {
  return mismatch(t.rbegin(), t.rend(), s.rbegin()).first == t.rend();
}

int main(int argc, char** argv) {
  arguments arguments;
  arguments.in = "-";
  arguments.decide = "fixed";
  arguments.learn = "1uip";
  arguments.forget = "nothing";
  arguments.backjump = true;
  arguments.minimize = false;
  arguments.phase_saving = true;
  arguments.dag = "";
  arguments.pebbling_graph = "";
  arguments.substitution_fn = "xor";
  arguments.substitution_arity = 2;
  arguments.viz = true;
  arguments.verbose = LOG_STATE_SUMMARY;
  argp_parse (&argp, argc, argv, 0, 0, &arguments);
  max_log_level = log_level(arguments.verbose);
  
  cnf f;
  if (arguments.in != string("-")) {
    LOG(LOG_ACTIONS) << "Parsing from " << arguments.in << endl;
    ifstream in(arguments.in);
    f = parse_dimacs(in);
  }
  else {
    LOG(LOG_ACTIONS) << "Parsing from stdin" << endl;
    f = parse_dimacs(cin);
  }

  cdcl_solver solver;
  solver.decide = arguments.decide;
  solver.learn = arguments.learn;
  solver.forget = arguments.forget;
  solver.backjump = arguments.backjump;
  solver.minimize = arguments.minimize;
  solver.phase_saving = arguments.phase_saving;

  if (not arguments.trace.empty()) {
    solver.trace.reset(new ofstream(arguments.trace));
    *solver.trace << "# -*- mode: conf -*-" << endl;
    *solver.trace << "batch 1" << endl;
  }

  if (not arguments.pebbling_graph.empty()) {
#ifndef NO_VIZ
    if (arguments.viz) {
    ifstream pebbling2(arguments.pebbling_graph);
    solver.vz.reset
      (new pebble_viz(pebbling2, arguments.substitution_fn,
                      arguments.substitution_arity));
    }
#endif
    ifstream pebbling(arguments.pebbling_graph);
    solver.pebble_helper.reset
      (new pebble(pebbling, arguments.substitution_fn,
                  arguments.substitution_arity));
  }

  LOG(LOG_ACTIONS) << "Start solving" << endl;
  proof proof = solver.solve(f);

  if (not arguments.dag.empty()) {
    ofstream dag(arguments.dag);
    if (endswith(arguments.dag, ".dot")) draw(dag, proof);
    else if (endswith(arguments.dag, ".tex")) tikz(dag, proof);
    else if (endswith(arguments.dag, ".asy")) asy(dag, proof);
    else {
      cerr << "Unknown output format" << endl;
      exit(1);
    }
  }
  measure(proof);
}

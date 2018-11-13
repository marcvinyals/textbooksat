#include <iostream>
#include <fstream>
#include <argp.h>

#include "dimacs.h"
#include "solver.h"
#include "analysis.h"
#include "log.h"
#ifndef NO_VIZ
#include "vizpebble.h"
#include "viztseitin.h"
#endif

using namespace std;

static argp_option options[] = {
  {"in", 'i', "FILE", 0,
   "Read formula in dimacs format from FILE (default: stdin)"},
  {"decide", 'd', "{fixed,reverse,vsids,random,ask}", 0,
   "Use the specified decision procedure (default: fixed)"},
  {"restart", 'r', "{none,always,luby}", 0,
   "Use the specified restart interval (default: none)"},
  {"learn", 'l', "{1uip,1uip-all,lastuip,decision}", 0,
   "Use the specified learning schema (default: 1uip)"},
  {"forget", 'f', "{nothing,everything,wide}", 0,
   "Use the specified forgetting schema (default: nothing)"},
  {"watch", 'w', "{reference,2wl}", 0,
   "Use the specified clause watcher (default: reference)"},
  {"decay", 6, "DOUBLE", 0,
   "Variable activity decay factor (default: 0.96875)"},
  {"bump", 7, "{learnt,conflict}", 0,
   "Use the specified bump plugin (default: conflict)"},
  {"clause-decay", 8, "DOUBLE", 0,
   "Clause activity decay factor (default: 0.99951171875)"},
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
   "Hint that the formula is the pebbbling of FILE (default: null)"},
  {"substitution-fn", 2, "{xor}", 0,
   "Hint that the formula was substituted with the specified function (default: xor)"},
  {"substitution-arity", 3, "INT", 0,
   "Hint that the formula was substituted with the specified arity (default: 2)"},
#ifndef NO_VIZ
  {"viz", 4, "BOOL", 0,
   "Visualize pebbling (default: 1)"},
  {"tseitin", 5, "BOOL", 0,
   "Visualize Tseitin (default: 0)"},
#endif
  {"verbose", 'v', "[0..3]", 0,
   "Verbosity level"},
  { 0 }
};

struct arguments {
  string in;
  string decide;
  string restart;
  string learn;
  string forget;
  string watcher;
  double decay;
  string bump;
  double clause_decay;
  bool backjump;
  bool minimize;
  bool phase_saving;
  string dag;
  string trace;
  string pebbling_graph;
  string substitution_fn;
  int substitution_arity;
  bool visualize_pebbling;
  bool visualize_tseitin;
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
  case 'r':
    arguments->restart = arg;
    break;
  case 'l':
    arguments->learn = arg;
    break;
  case 'f':
    arguments->forget = arg;
    break;
  case 'w':
    arguments->watcher = arg;
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
    arguments->visualize_pebbling = atoi(arg);
    break;
  case 5:
    arguments->visualize_tseitin = atoi(arg);
    break;
  case 6:
    arguments->decay = atof(arg);
    break;
  case 7:
    arguments->bump = arg;
    break;
  case 8:
    arguments->clause_decay = atof(arg);
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
  arguments.restart = "none";
  arguments.learn = "1uip";
  arguments.forget = "nothing";
  arguments.watcher = "reference";
  arguments.decay = 1.-1./32.;
  arguments.bump = "conflict";
  arguments.clause_decay = 1.-1./2048.;
  arguments.backjump = true;
  arguments.minimize = false;
  arguments.phase_saving = true;
  arguments.dag = "";
  arguments.pebbling_graph = "";
  arguments.substitution_fn = "xor";
  arguments.substitution_arity = 2;
  arguments.visualize_pebbling = true;
  arguments.visualize_tseitin = false;
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
  solver.restart = arguments.restart;
  solver.learn = arguments.learn;
  solver.forget = arguments.forget;
  solver.watcher = arguments.watcher;
  solver.bump = arguments.bump;
  solver.decay = arguments.decay;
  solver.clause_decay = arguments.clause_decay;
  solver.backjump = arguments.backjump;
  solver.minimize = arguments.minimize;
  solver.phase_saving = arguments.phase_saving;

  if (not arguments.trace.empty()) {
    solver.trace = shared_ptr<ostream>(new ofstream(arguments.trace));
    *solver.trace << "# -*- mode: conf -*-" << endl;
    *solver.trace << "batch 1" << endl;
  }

#ifndef NO_VIZ
  if (not arguments.pebbling_graph.empty()) {
    if (arguments.visualize_pebbling) {
      ifstream pebbling(arguments.pebbling_graph);
      solver.vz.reset
        (new pebble_viz(pebbling, arguments.substitution_fn, arguments.substitution_arity));
    }
  }
  if (arguments.visualize_tseitin) {
    solver.vz.reset(new tseitin_viz(f));
  }
#endif

  LOG(LOG_ACTIONS) << "Start solving" << endl;
  proof proof = solver.solve(f);

  if (not arguments.dag.empty()) {
    ofstream dag(arguments.dag);
    if (endswith(arguments.dag, ".dot")) draw(dag, proof);
    else if (endswith(arguments.dag, ".beamer.tex")) tikz(dag, proof, true);
    else if (endswith(arguments.dag, ".tex")) tikz(dag, proof);
    else if (endswith(arguments.dag, ".asy")) asy(dag, proof);
    else if (endswith(arguments.dag, ".rup")) rup(dag, proof);
    else {
      cerr << "Unknown output format" << endl;
      exit(1);
    }
  }
  measure(proof);
}

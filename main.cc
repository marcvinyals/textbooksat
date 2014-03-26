#include "dimacs.h"
#include "cdcl.h"
#include "analysis.h"

#include <iostream>
#include <fstream>
#include <argp.h>
using namespace std;

static argp_option options[] = {
  {"in", 'i', "FILE", 0,
   "Read formula in dimacs format from FILE (default: stdin)"},
  {"decide", 'd', "{fixed,reverse,ask}", 0,
   "Use the specified decision procedure (default: fixed"},
  {"learn", 'l', "{1uip,1uip-all}", 0,
   "Use the specified learning schema (default: 1uip"},
  {"backjump", 'b', "BOOL", 0,
   "On a conflict, backtrack deeper than the decision level as long as the "
   "learnt clause is unit (default: 0)"},
  {"minimize", 'm', "BOOL", 0,
   "Try to subsume the learnt clause by resolving it with some other "
   "clause in the database (default: 0)"},
  {"proof-dag", 'p', "FILE", 0,
   "Output the proof dag to FILE (default: null)"},
  { 0 }
};

struct arguments {
  string in;
  string decide;
  string learn;
  bool backjump;
  bool minimize;
  string dag;
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
  case 'b':
    arguments->backjump = atoi(arg);
    break;
  case 'm':
    arguments->minimize = atoi(arg);
    break;
  case 'p':
    arguments->dag = arg;
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, 0, 0 };
  
int main(int argc, char** argv) {
  arguments arguments;
  arguments.in = "-";
  arguments.decide = "fixed";
  arguments.learn = "1uip";
  argp_parse (&argp, argc, argv, 0, 0, &arguments);
  
  cnf f;
  if (arguments.in != string("-")) {
    cerr << "Parsing from " << arguments.in << endl;
    ifstream in(arguments.in);
    f = parse_dimacs(in);
  }
  else {
    cerr << "Parsing from stdin" << endl;
    f = parse_dimacs(cin);
  }

  cdcl_solver solver;
  solver.decide = arguments.decide;
  solver.learn = arguments.learn;
  solver.backjump = arguments.backjump;
  solver.minimize = arguments.minimize;

  cerr << "Start solving" << endl;
  proof proof = solver.solve(f);

  if (not arguments.dag.empty()) {
    ofstream dag(arguments.dag);
    draw(dag, proof);
  }
  measure(proof);
}

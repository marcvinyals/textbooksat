#include "dimacs.h"
#include "cdcl.h"

#include <iostream>
#include <fstream>
#include <argp.h>
using namespace std;

static argp_option options[] = {
  {"in", 'i', "INPUT", 0, "Read formula in dimacs format from INPUT"},
  {"decide", 'd', "DECIDE", 0, "Use the decision procedure DECIDE"},
  {"learn", 'l', "LEARN", 0, "Use the learning schema LEARN"},
  { 0 }
};

struct arguments {
  string in;
  string decide;
  string learn;
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
  cerr << "Start solving" << endl;
  solver.solve(f);
}

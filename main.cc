#include "dimacs.h"
#include "cdcl.h"

#include <iostream>
#include <fstream>
using namespace std;

int main(int argc, char** argv) {
  cerr << "Start parsing" << endl;
  cnf f;
  if (argc>1) {
    ifstream in(argv[1]);
    f = parse_dimacs(in);
  }
  else f = parse_dimacs(cin);
  cdcl_solver solver;
  if (argc>2) solver.decide = argv[2];
  cerr << "Start solving" << endl;
  solver.solve(f);
}

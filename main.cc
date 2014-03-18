#include "dimacs.h"
#include "cdcl.h"

#include <iostream>
#include <fstream>
using namespace std;

int main(int argc, char** argv) {
  cnf f;
  if (argc>1 and argv[1]!=string("-")) {
    cerr << "Parsing from " << argv[1] << endl;
    ifstream in(argv[1]);
    f = parse_dimacs(in);
  }
  else {
    cerr << "Parsing from stdin" << endl;
    f = parse_dimacs(cin);
  }
  cdcl_solver solver;
  if (argc>2) solver.decide = argv[2];
  cerr << "Start solving" << endl;
  solver.solve(f);
}

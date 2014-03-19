#include "dimacs.h"
#include <sstream>
#include <cassert>

#include <iostream>
using namespace std;

void parse_header(istream& in, int& nvars, int& nclauses) {
  string s;
  while(getline(in, s) and (s.empty() or s[0] == 'c'));
  istringstream ss(s);
  char p;
  string t;
  ss >> p >> t >> nvars >> nclauses;
  if (p!='p' or t!="cnf") {
    cerr << "Not a dimacs file" << endl;
    exit(1);
  }
}

cnf parse_dimacs(istream& in) {
  cnf f;
  int nclauses;
  parse_header(in, f.variables, nclauses);
  string s;
  while(getline(in, s)) {
    istringstream ss(s);
    clause c;
    int x;
    while(ss >> x and x) {
      c.literals.push_back(from_dimacs(x));
    }
    if (x!=0) {
      cerr << "Not a zero-terminated line" << endl;
      exit(1);
    }
    f.clauses.push_back(c);
  }
  return f;
}

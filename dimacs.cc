#include "dimacs.h"

#include <sstream>
#include <cassert>
#include <set>
#include <iostream>
using namespace std;

class parser {
public:
  parser(istream& in);
  operator cnf() { return f; }
private:
  cnf f;
  istream& in;
  int& nvars;
  int nclauses;
  unordered_map<int, string>& variable_names;
  int lineno;
  void parse_header();
  void parse_body();
};

#define ERROR(message) {\
    cerr << "Error on line " << lineno << ": " << message << endl;\
    exit(1);}
#define WARNING(message) {\
    cerr << "Warning on line " << lineno << ": " << message << endl;}

void parser::parse_header() {
  string s;
  for (; getline(in, s); ++lineno) {
    if (s.empty()) continue;
    if (s[0]=='c') {
      stringstream ss(s);
      char c;
      int varnum;
      string label,varname;
      ss >> c >> label >> varnum >> varname;
      if (label=="varname") variable_names[varnum-1]=varname;
    }
    else break;
  }
  istringstream ss(s);
  char p;
  string t;
  ss >> p >> t >> nvars >> nclauses;
  if (p!='p' or t!="cnf") ERROR("header not found");
}

void parser::parse_body() {
  string s;
  for (; getline(in, s); ++lineno) {
    istringstream ss(s);
    set<literal> c;
    int x;
    while(ss >> x and x) {
      literal l=from_dimacs(x);
      if (c.count(l)) WARNING("repeated literal " << x);
      if (c.count(~l)) WARNING("opposite literals " << x << " and " << (~x));
      variable v=variable(l);
      if (v >= nvars) {
        WARNING("variable " << v << " out of range");
        nvars=v+1;
      }
      c.insert(l);
    }
    if (x!=0) ERROR("not zero-terminated");
    f.clauses.push_back(vector<literal>(c.begin(), c.end()));
  }
  if (f.clauses.size() != nclauses) {
    WARNING("expected " << nclauses << " clauses but read " << f.clauses.size());
  }
}

parser::parser(istream& in) :
  in(in), nvars(f.variables),
  variable_names(f.variable_names), lineno(1) {
  parse_header();
  parse_body();
}

cnf parse_dimacs(istream& in) {
  return parser(in);
}

#include "pebble_util.h"

#include <iostream>
#include <sstream>

#include "data_structures.h"

using namespace std;

int parse_header(istream& in) {
  while(in) {
    string line;
    getline(in, line);
    if (line.empty() || line[0] == 'c') continue;
    stringstream ss(line);
    int n;
    ss >> n;
    if (n<0) break;
    return n;
  }
  cerr << "could not parse graph header" << endl;
  exit(1);
}

vector<vector<int>> parse_kth(istream& in) {
  int n = parse_header(in);
  vector<vector<int>> g(n);
  while(in) {
    string line;
    getline(in, line);
    if (line.empty() || line[0] == 'c') continue;
    stringstream ss(line);
    int uu;
    char c;
    ss >> uu >> c;
    uu--;
    if (uu<0 || uu>=n || c!=':') {
      cerr << "invalid node" << endl;
      exit(1);
    }
    vector<int>& u = g[uu];
    int v;
    while(ss >> v) {
      v--;
      if (v<0 || v>=n) {
        cerr << "invalid neighbour" << endl;
        exit(1);
      }
      u.push_back(v);
    }
  }
  return g;
}

set<vector<int>> filter(const set<vector<int>>& assignments) {
  set<vector<int>> ret;
  for (const vector<int>& a : assignments) {
    vector<int> b(a);
    for (int& x : b) {
      if (not x) continue;
      int y=x;
      x=0;
      if (assignments.count(b)) goto redundant;
      x=y;
    }
    ret.insert(a);
  redundant:;
  }
  return ret;
}

substitution::substitution(const string& fn, int arity) : arity(arity) {
  int mmax=1;
  for (int i=0; i<arity; ++i) mmax*=3;
  for (int m=0; m<mmax; ++m) {
    vector<int> a(arity);
    for (int i=0, mm=m; i<arity; ++i, mm/=3) a[i]=mm%3-1;
    int is_true = 0;
    if (fn == "xor") {
      is_true=-1;
      for (int x : a) is_true*=-x;
    }
    else if (fn == "maj") {
      vector<int> n(3);
      for (int x : a) n[x+1]++;
      if (n[0]*2>arity) is_true=-1;
      else if (n[2]*2>arity) is_true=1;
    }
    else {
      cerr << "Invalid substitution function" << endl;
      exit(1);
    }
    if (is_true==1) true_assignments.insert(a);
    else if (is_true==-1) false_assignments.insert(a);
  }
  true_minimal = filter(true_assignments);
  false_minimal = filter(false_assignments);
}

bool substitution::is_clause(const clause& c, int& u, int& value) {
  if (c.width() > arity) return false;
  vector<int> forbidden(arity);
  u = variable(*c.begin())/arity;
  for (literal l : c) {
    if (variable(l)/arity !=u) return false;
    forbidden[variable(l)%arity] = (1-l.polarity()*2);
  }
  if (true_minimal.count(forbidden)) value=-1;
  else if (false_minimal.count(forbidden)) value=1;
  else return false;
  return true;
}

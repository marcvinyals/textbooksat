#include "viz.h"

#include <cassert>
#include <set>
#include <sstream>
using namespace std;

vector<vector<int>> parse_kth(istream& in) {
  int n;
  in >> n;
  string s;
  getline(in,s);
  vector<vector<int>> g(n);
  for(auto &u:g) {
    getline(in, s);
    stringstream ss(s);
    int uu;
    char c;
    ss >> uu >> c;
    uu--;
    assert(uu>=0);
    assert(uu<n);
    assert (c==':');
    int v;
    while(ss >> v) {
      v--;
      assert(v>=0);
      assert(v<uu);
      u.push_back(v);
    }
  }
  return g;
}

pebble_viz::pebble_viz(istream& graph) : g(parse_kth(graph)) {}

void pebble_viz::draw_pebbling(const vector<int>& a) const {
  stringstream out;
  out << "digraph g {" << endl;
  out << "node [ style = filled, color=white ];" << endl;
  for (int u=0; u<g.size(); ++u) {
    for (int v : g[u]) {
      out << 'x' << u << " -> " << 'x' << v << ';' << endl;
    }
  }
  set<vector<int>> true_assignments({{1,-1},{-1,1}});
  set<vector<int>> false_assignments({{1,1},{-1,-1}});
  for (int u=0; u<g.size(); ++u) {
    vector<int> a_u({a[2*u],a[2*u+1]});
    if (true_assignments.count(a_u)) {
      out << 'x' << u << " [color=green];" << endl;
    }
    else if (false_assignments.count(a_u)) {
      out << 'x' << u << " [color=red];" << endl;
    }
    else if (a_u != vector<int>({0,0})) {
      out << 'x' << u << " [color=grey];" << endl;
    }
  }
  out << '}' << endl;

  FILE* dotpipe = popen ("dot -Txlib", "w");
  fputs(out.str().c_str(), dotpipe);
  pclose(dotpipe);
}

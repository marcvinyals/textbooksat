#include "viz.h"

#include <cassert>
#include <set>
#include <sstream>

#include <graphviz/gvc.h>

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

pebble_viz::pebble_viz(istream& graph) :
  tmpfile(tmpnam(nullptr)) {
  vector<vector<int>> adjacency = parse_kth(graph);
  int n = adjacency.size();

  gvc = gvContext();
#ifdef STONE_AGE
  g = agopen("pebble", AGDIGRAPHSTRICT);
  agnodeattr(g, "style", "filled");
  agnodeattr(g, "color", "white");
#else
  g = agopen("pebble", Agstrictdirected, NULL);
  agattr(g, AGNODE, "style", "filled");
  agattr(g, AGNODE, "color", "white");
#endif
  nodes.reserve(n);
  for (int u=0; u<n; ++u) {
    char x[10];
    snprintf(x, 10, "%d", u+1);
#ifdef STONE_AGE
    nodes.push_back(agnode(g, x));
#else
    nodes.push_back(agnode(g, x, TRUE));
#endif
  }
  for (int u=0; u<n; ++u) {
    for (int v : adjacency[u]) {
#ifdef STONE_AGE
      agedge(g, nodes[u], nodes[v]);
#else
      agedge(g, nodes[u], nodes[v], NULL, TRUE);
#endif
    }
  }
}

pebble_viz::~pebble_viz() {
  agclose(g);
  gvFreeContext(gvc);
  unlink(tmpfile.c_str());
}

void pebble_viz::draw_pebbling(const vector<int>& a) const {
  set<vector<int>> true_assignments({{1,-1},{-1,1}});
  set<vector<int>> false_assignments({{1,1},{-1,-1}});

  for (int u=0; u<nodes.size(); ++u) {
    vector<int> a_u({a[2*u],a[2*u+1]});
    string color = "white";
    if (true_assignments.count(a_u)) color = "green";
    else if (false_assignments.count(a_u)) color = "red";
    else if (a_u != vector<int>({0,0})) color = "grey";
    agset(nodes[u], "color", (char*)color.c_str());
  }

  gvLayout(gvc, g, "dot");
  gvRenderFilename (gvc, g, "png", tmpfile.c_str());
  gvFreeLayout(gvc, g);
  
  string cmd = "display -remote " + tmpfile;
  system(cmd.c_str());
  sleep(1);
}

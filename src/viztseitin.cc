#include "viztseitin.h"

#include <algorithm>
#include <set>

#include <graphviz/gvc.h>

#include "data_structures.h"
using namespace std;

tseitin_viz::tseitin_viz(const cnf& f) {
  g = agopen(const_cast<char*>("tseitin"), Agundirected, NULL);
  agattr(g, AGNODE, const_cast<char*>("label"), const_cast<char*>(""));
  agattr(g, AGEDGE, const_cast<char*>("label"), const_cast<char*>(""));
  agattr(g, AGEDGE, const_cast<char*>("color"), const_cast<char*>("black"));

  vector<set<variable>> edgesof;
  for (auto it = f.clauses.begin(); it != f.clauses.end();) {
    nodes.push_back(agnode(g, NULL, TRUE));
    edgesof.push_back(set<variable>(it->dom_begin(), it->dom_end()));
    it+=(1ull<<(it->width()-1));
  }
  edges.resize(f.variables);
  for (unsigned i=0; i<edgesof.size(); ++i) {
    for (unsigned j=i+1; j<edgesof.size(); ++j) {
      vector<variable> edgesij;
      set_intersection(edgesof[i].begin(), edgesof[i].end(),
                       edgesof[j].begin(), edgesof[j].end(),
                       back_inserter(edgesij));
      for (variable e : edgesij) {
        char x[10];
        snprintf(x, 10, "%d", e+1);
        edges[e] = agedge(g, nodes[i], nodes[j], NULL, TRUE);
        agset(edges[e], const_cast<char*>("label"), x);
      }
    }
  }
  gvLayout(gvc, g, "neato");
  render();
}

void tseitin_viz::draw(const vector<int>& a,
                       const vector<restricted_clause>& mem) {
  const string colors[] = {"red", "black", "green"};
  for (size_t e=0; e<edges.size(); ++e) {
    agset(edges[e], const_cast<char*>("color"),
          const_cast<char*>(colors[a[e]+1].c_str()));
  }
  render();
}

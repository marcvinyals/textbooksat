#include "viz.h"

#include <cassert>
#include <set>
#include <sstream>

#include <graphviz/gvc.h>

#include "cdcl.h"

using namespace std;
using namespace cimg_library;

pebble_viz::pebble_viz(istream& graph, const string& fn, int arity) :
  sub(fn, arity),
  tmpfile(tmpnam(nullptr)) {
  vector<vector<int>> adjacency = parse_kth(graph);
  int n = adjacency.size();

  gvc = gvContext();
  g = agopen(const_cast<char*>("pebble"), Agstrictdirected, NULL);
  agattr(g, AGRAPH, const_cast<char*>("rankdir"), const_cast<char*>("LR"));
  agattr(g, AGNODE, const_cast<char*>("style"), const_cast<char*>("filled"));
  agattr(g, AGNODE, const_cast<char*>("color"), const_cast<char*>("black"));
  agattr(g, AGNODE, const_cast<char*>("fillcolor"), const_cast<char*>("white"));
  agattr(g, AGNODE, const_cast<char*>("penwidth"), const_cast<char*>("1"));
  nodes.reserve(n);
  for (int u=0; u<n; ++u) {
    char x[10];
    snprintf(x, 10, "%d", u+1);
    nodes.push_back(agnode(g, x, TRUE));
  }
  for (int u=0; u<n; ++u) {
    for (int v : adjacency[u]) {
      agedge(g, nodes[v], nodes[u], NULL, TRUE);
    }
  }
  gvLayout(gvc, g, "dot");
  render();
}

pebble_viz::~pebble_viz() {
  gvFreeLayout(gvc, g);
  agclose(g);
  gvFreeContext(gvc);
  unlink(tmpfile.c_str());
}

void pebble_viz::draw_assignment(const vector<int>& a) {
  for (size_t u=0; u<nodes.size(); ++u) {
    vector<int> a_u(a.begin()+sub.arity*u, a.begin()+sub.arity*(u+1));
    string color = "white";
    if (sub.true_assignments.count(a_u)) color = "olivedrab1";
    else if (sub.false_assignments.count(a_u)) color = "salmon1";
    else if (a_u != vector<int>(sub.arity,0)) color = "grey";
    agset(nodes[u], const_cast<char*>("fillcolor"),
          const_cast<char*>(color.c_str()));
  }
}

void pebble_viz::draw_learnt(const vector<restricted_clause>& mem) {
  static size_t lastcfgsz = 0;
  if (mem.size() != lastcfgsz) {
    lastcfgsz = mem.size();
    vector<int> truth(nodes.size());
    for (const restricted_clause r : mem) {
      int key, value;
      if (sub.is_clause(r.source->c, key, value)) truth[key]+=value;
    }
    vector<string> border_map = {"red3", "red1", "black", "green1", "green3"};
    for (size_t u=0; u<nodes.size(); ++u) {
      int tcap = truth[u];
      tcap = min(tcap,2);
      tcap = max(tcap,-2);
      agset(nodes[u], const_cast<char*>("color"),
            const_cast<char*>(border_map[tcap+2].c_str()));
      stringstream penwidth;
      penwidth << abs(truth[u]);
      agset(nodes[u], const_cast<char*>("penwidth"),
            const_cast<char*>(penwidth.str().c_str()));
    }
  }
}

void pebble_viz::render() {
  gvRenderFilename (gvc, g, "bmp", tmpfile.c_str());
  CImg<unsigned char> image(tmpfile.c_str());
  display.display(image);
  usleep(100000);
}

void pebble_viz::draw(const vector<int>& a,
                      const vector<restricted_clause>& mem) {
  draw_assignment(a);
  draw_learnt(mem);
  render();
}

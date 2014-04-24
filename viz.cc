#include "viz.h"

#include <cassert>
#include <set>
#include <sstream>

#include <graphviz/gvc.h>

#include "cdcl.h"

using namespace std;
using namespace cimg_library;

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
  g = agopen("pebble", Agstrictdirected, NULL);
  agattr(g, AGNODE, "style", "filled");
  agattr(g, AGNODE, "color", "black");
  agattr(g, AGNODE, "fillcolor", "white");
  agattr(g, AGNODE, "penwidth", "1");
  nodes.reserve(n);
  for (int u=0; u<n; ++u) {
    char x[10];
    snprintf(x, 10, "%d", u+1);
    nodes.push_back(agnode(g, x, TRUE));
  }
  for (int u=0; u<n; ++u) {
    for (int v : adjacency[u]) {
      agedge(g, nodes[u], nodes[v], NULL, TRUE);
    }
  }
}

pebble_viz::~pebble_viz() {
  agclose(g);
  gvFreeContext(gvc);
  unlink(tmpfile.c_str());
}

set<vector<int>> true_assignments({{1,-1},{-1,1}});
set<vector<int>> false_assignments({{1,1},{-1,-1}});

void pebble_viz::draw_assignment(const vector<int>& a) {
  for (size_t u=0; u<nodes.size(); ++u) {
    vector<int> a_u({a[2*u],a[2*u+1]});
    string color = "white";
    if (true_assignments.count(a_u)) color = "olivedrab1";
    else if (false_assignments.count(a_u)) color = "salmon1";
    else if (a_u != vector<int>({0,0})) color = "grey";
    agset(nodes[u], "fillcolor", (char*)color.c_str());
  }
}

void pebble_viz::draw_learnt(const vector<restricted_clause>& mem) {
  static size_t lastcfgsz = 0;
  if (mem.size() != lastcfgsz) {
    lastcfgsz = mem.size();
    vector<int> truth(nodes.size());
    for (const restricted_clause r : mem) {
      const clause& c = r.source->c;
      if (c.width() == 2) {
        auto it = c.begin();
        literal l1 = *it;
        variable x = variable(l1);
        ++it;
        literal l2 = *it;
        if (x/2 == variable(l2)/2) {
          vector<int> polarities({l1.polarity()*2-1, l2.polarity()*2-1});
          string border = "black";
          if (true_assignments.count(polarities)) truth[x/2]--;
          else if (false_assignments.count(polarities)) truth[x/2]++;
        }
      }
    }
    vector<string> border_map = {"red3", "red1", "black", "green1", "green3"};
    vector<string> border_width = {"3", "2", "1", "2", "3"};
    for (size_t u=0; u<nodes.size(); ++u) {
      agset(nodes[u], "color", (char*)border_map[truth[u]+2].c_str());    
      agset(nodes[u], "penwidth", (char*)border_width[truth[u]+2].c_str());    
    }
  }
}

void pebble_viz::draw(const vector<int>& a,
                      const vector<restricted_clause>& mem) {
  draw_assignment(a);
  draw_learnt(mem);
  
  gvLayout(gvc, g, "dot");
  gvRenderFilename (gvc, g, "png", tmpfile.c_str());
  gvFreeLayout(gvc, g);

  CImg<unsigned char> image(tmpfile.c_str());
  display.display(image);
}

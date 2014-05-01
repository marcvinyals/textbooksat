#include "viz.h"

#include <cassert>
#include <set>
#include <sstream>

#include <graphviz/gvc.h>

#include "cdcl.h"

using namespace std;
using namespace cimg_library;

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
  cerr << "could not parse header" << endl;
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

void pebble_viz::make_assignments(const string& fn, int arity) {
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

pebble_viz::pebble_viz(istream& graph, const string& fn, int arity) :
  arity(arity),
  tmpfile(tmpnam(nullptr)) {
  make_assignments(fn, arity);

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
    vector<int> a_u(a.begin()+arity*u, a.begin()+arity*(u+1));
    string color = "white";
    if (true_assignments.count(a_u)) color = "olivedrab1";
    else if (false_assignments.count(a_u)) color = "salmon1";
    else if (a_u != vector<int>(arity,0)) color = "grey";
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
      const clause& c = r.source->c;
      if (c.width() <= arity) {
        vector<int> forbidden(arity);
        int u = variable(*c.begin())/arity;
        for (literal l : c) {
          if (variable(l)/arity !=u) goto nonuniform;
          forbidden[variable(l)%arity] = (1-l.polarity()*2);
        }
        if (true_minimal.count(forbidden)) truth[u]--;
        else if (false_minimal.count(forbidden)) truth[u]++;
      nonuniform:;
      }
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

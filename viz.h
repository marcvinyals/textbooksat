#pragma once

#include <istream>
#include <vector>
#include <set>

#include <CImg.h>

typedef struct Agraph_s Agraph_t;
typedef struct Agnode_s Agnode_t;
typedef struct GVC_s GVC_t;
class restricted_clause;

class pebble_viz {
 public:
  pebble_viz(std::istream& graph, int arity);
  ~pebble_viz();
  pebble_viz(const pebble_viz&) = delete;
  void draw(const std::vector<int>& a,
            const std::vector<restricted_clause>& mem);
 private:
  void draw_assignment(const std::vector<int>& a);
  void draw_learnt(const std::vector<restricted_clause>& mem);
  void render();

  int arity;
  std::set<std::vector<int>> true_assignments;
  std::set<std::vector<int>> false_assignments;
  
  std::string tmpfile;
  GVC_t* gvc;
  Agraph_t* g;  
  cimg_library::CImgDisplay display;
  std::vector<Agnode_t*> nodes;
};

#pragma once

#include <istream>
#include <vector>
#include <CImg.h>

typedef struct Agraph_s Agraph_t;
typedef struct Agnode_s Agnode_t;
typedef struct GVC_s GVC_t;

class pebble_viz {
 public:
  pebble_viz(std::istream& graph);
  ~pebble_viz();
  pebble_viz(const pebble_viz&) = delete;
  void draw_pebbling(const std::vector<int>& a);
 private:
  std::string tmpfile;
  GVC_t* gvc;
  Agraph_t* g;  
  cimg_library::CImgDisplay display;
  std::vector<Agnode_t*> nodes;
};

#pragma once

#include <istream>
#include <vector>

struct Agraph_t;
struct Agnode_t;
typedef struct GVC_s GVC_t;

class pebble_viz {
 public:
  pebble_viz(std::istream& graph);
  ~pebble_viz();
  pebble_viz(const pebble_viz&) = delete;
  void draw_pebbling(const std::vector<int>& a) const;
 private:
  std::string tmpfile;
  GVC_t* gvc;
  Agraph_t* g;
  std::vector<Agnode_t*> nodes;
};

#pragma once

#include <string>

#include <CImg.h>

typedef struct Agraph_s Agraph_t;
typedef struct Agnode_s Agnode_t;
typedef struct GVC_s GVC_t;

class graphviz_viz {
 public:
  virtual ~graphviz_viz();
  graphviz_viz(const graphviz_viz&) = delete;
 protected:
  graphviz_viz();
  void render();  
  GVC_t* gvc; // Owned
  Agraph_t* g; // Owned, created by subclass
 private:
  std::string tmpfile;
  cimg_library::CImgDisplay display;
};

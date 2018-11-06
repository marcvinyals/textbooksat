#pragma once

#ifdef NO_VIZ

class graphviz_viz;

#else

#include <string>
#include <vector>

#include <CImg.h>

#include "data_structures.h"

typedef struct Agraph_s Agraph_t;
typedef struct Agnode_s Agnode_t;
typedef struct Agedge_s Agedge_t;
typedef struct GVC_s GVC_t;

class graphviz_viz {
 public:
  graphviz_viz();
  virtual ~graphviz_viz();
  graphviz_viz(const graphviz_viz&) = delete;
  virtual void draw(const std::vector<int>& a,
                    const std::vector<restricted_clause>& mem) = 0;
 protected:
  void render();  
  GVC_t* gvc; // Owned
  Agraph_t* g; // Owned, created by subclass
 private:
  std::string tmpfile;
  cimg_library::CImgDisplay display;
};

#endif

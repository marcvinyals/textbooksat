#pragma once

#include <vector>

#include "viz.h"
#include "data_structures.h"

class tseitin_viz : public graphviz_viz {
 public:
  tseitin_viz(const cnf& f);
  virtual void draw(const std::vector<int>& a,
            const std::vector<restricted_clause>& mem);
 private:
  std::vector<Agnode_t*> nodes;
  std::vector<Agedge_t*> edges;
};

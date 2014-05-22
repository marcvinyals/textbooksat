#pragma once

#include <istream>
#include <vector>

#include "pebble_util.h"
#include "viz.h"

class pebble_viz : public graphviz_viz {
 public:
  pebble_viz(std::istream& graph, const std::string& fn, int arity);
  virtual void draw(const std::vector<int>& a,
                    const std::vector<restricted_clause>& mem);
 private:
  void draw_assignment(const std::vector<int>& a);
  void draw_learnt(const std::vector<restricted_clause>& mem);

  substitution sub;

  std::vector<Agnode_t*> nodes;
};

#pragma once

#include <istream>
#include <vector>

class pebble_viz {
 public:
  pebble_viz(std::istream& graph);
  void draw_pebbling(const std::vector<int>& a) const;
 private:
  std::vector<std::vector<int>> g;
};

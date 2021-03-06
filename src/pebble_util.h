#pragma once

#include <istream>
#include <set>
#include <string>
#include <vector>

#include "data_structures.h"

typedef std::set<std::vector<int>> assignment_set;
class substitution {
 public:
  substitution(const std::string& fn, unsigned int arity);
  unsigned int arity;
  assignment_set true_assignments;
  assignment_set false_assignments;
  bool is_clause(const clause& c, unsigned int& key, int& value);
 private:
  assignment_set true_minimal;
  assignment_set false_minimal;
};

std::vector<std::vector<int>> parse_kth(std::istream& in);

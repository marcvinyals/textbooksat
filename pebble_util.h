#pragma once

#include <istream>
#include <set>
#include <string>
#include <vector>

class clause;
class literal;

typedef std::set<std::vector<int>> assignment_set;
class substitution {
 public:
  substitution(const std::string& fn, int arity);
  int arity;
  assignment_set true_assignments;
  assignment_set false_assignments;
  bool is_clause(const clause& c, int& key, int& value);
 private:
  assignment_set true_minimal;
  assignment_set false_minimal;
};

std::vector<std::vector<int>> parse_kth(std::istream& in);

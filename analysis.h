#include "data_structures.h"

#include <list>
#include <vector>
#include <ostream>

void measure(const std::vector<proof_clause>& formula,
             const std::list<proof_clause>& proof);
void draw(std::ostream& out,
          const std::vector<proof_clause>& formula,
          const std::list<proof_clause>& proof);

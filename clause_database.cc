#include "clause_database.h"

#include "propagation_queue.h"

template<typename T>
void clause_database<T>::fill_propagation_queue() {
  for (const auto& c : working_clauses) {
    if (c.unit() and not assignment[variable(c.propagate().to)]) {
      c.assert_unit(assignment);
      propagation_queue.propagate(c);
    }
  }
}

#include "reference_clause_database.h"
#include "watched_clause_database.h"

template class clause_database<eager_restricted_clause>;
template class clause_database<lazy_restricted_clause>;
template class clause_database<watched_clause>;

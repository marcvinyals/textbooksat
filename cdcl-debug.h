struct double_restricted_clause : eager_restricted_clause {
  lazy_restricted_clause alt;
  double_restricted_clause(const proof_clause& c) : eager_restricted_clause(c), alt(c) {}

  bool unit() const {
    if (eager_restricted_clause::unit() != alt.unit()) {
      std::cerr << satisfied << ' ' << alt.satisfied << std::endl;
      std::cerr << literals.size() << ' ' << alt.unassigned << std::endl;
    }
    assert(eager_restricted_clause::unit()==alt.unit());
    return eager_restricted_clause::unit();
  }

  bool contradiction() const {
    assert(eager_restricted_clause::contradiction()==alt.contradiction());
    return eager_restricted_clause::contradiction();
  }

  branch propagate() const {
    assert(eager_restricted_clause::propagate().to==alt.propagate().to);
    return eager_restricted_clause::propagate();
  }
  
  void restrict(literal l) {
    eager_restricted_clause::restrict(l);
    alt.restrict(l);
    assert(bool(satisfied)==alt.satisfied);
    if (not satisfied) assert(literals.size()==alt.unassigned);
  }

  void loosen(literal l) {
    eager_restricted_clause::loosen(l);
    alt.loosen(l);
    assert(bool(satisfied)==alt.satisfied);
    if (not satisfied) assert(literals.size()==alt.unassigned);
  }

  void restrict_to_unit(const std::vector<int>& assignment) {
    eager_restricted_clause::restrict_to_unit(assignment);
    alt.restrict_to_unit(assignment);
  }

  void reset() {
    eager_restricted_clause::reset();
    alt.reset();
  }
};

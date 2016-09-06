#include "data_structures.h"
#include "solver.h"
#include "dimacs.h"

#include <gtest/gtest.h>

#include <tuple>

using namespace std;
using testing::Test;
using testing::TestWithParam;
using testing::Combine;
using testing::Values;
using testing::Bool;

TEST(ClauseTest, subsumes) {
  clause c;
  clause d(vector<literal>({from_dimacs(1)}));
  EXPECT_TRUE(c.subsumes(d));
  EXPECT_FALSE(d.subsumes(c));
}

TEST(ClauseTest, resolve_empty) {
  clause c(vector<literal>({from_dimacs(1)}));
  clause d(vector<literal>({from_dimacs(-1)}));
  clause e;
  EXPECT_TRUE(e == resolve(c,d));
}

TEST(ClauseTest, resolve_disjoint) {
  clause c(vector<literal>({from_dimacs(1), from_dimacs(2)}));
  clause d(vector<literal>({from_dimacs(-1), from_dimacs(-3)}));
  clause e(vector<literal>({from_dimacs(2), from_dimacs(-3)}));
  EXPECT_TRUE(e == resolve(c,d));
}

TEST(ClauseTest, resolve_same) {
  clause c(vector<literal>({from_dimacs(1), from_dimacs(2)}));
  clause d(vector<literal>({from_dimacs(-1), from_dimacs(2)}));
  clause e(vector<literal>({from_dimacs(2)}));
  EXPECT_TRUE(e == resolve(c,d));
}

TEST(ClauseTest, resolve_fail_empty) {
  clause c,d;
  EXPECT_DEATH(resolve(c,d),"");
}

TEST(ClauseTest, resolve_fail_self) {
  clause c(vector<literal>({from_dimacs(1)}));
  clause d(vector<literal>({from_dimacs(1)}));
  EXPECT_DEATH(resolve(c,d),"");
}

class SolverTest : public Test {
protected:
  cdcl_solver solver;
  void SetUp() {
    solver.decide = "fixed";
    solver.learn = "1uip";
    solver.forget = "nothing";
    solver.bump = "learnt";
    solver.backjump = true;
    solver.minimize = false;
    solver.phase_saving = false;
  }
};

TEST_F(SolverTest, empty) {
  istringstream s("p cnf 0 0\n");
  cnf f = parse_dimacs(s);
  ASSERT_EXIT(solver.solve(f), ::testing::ExitedWithCode(0), "");
}

TEST_F(SolverTest, contradiction) {
  istringstream s("p cnf 0 1\n0\n");
  cnf f = parse_dimacs(s);
  ASSERT_DEATH(solver.solve(f), "");
}

TEST_F(SolverTest, sat) {
  istringstream s("p cnf 1 1\n1 0\n");
  cnf f = parse_dimacs(s);
  ASSERT_EXIT(solver.solve(f), ::testing::ExitedWithCode(0), "");
}

TEST_F(SolverTest, unit) {
  istringstream s("p cnf 1 2\n1 0\n-1 0\n");
  cnf f = parse_dimacs(s);
  proof pi = solver.solve(f);
  EXPECT_EQ(pi.resolution.size(), 1);
}


typedef tuple<const char*, // decide
              const char*, // learn
              const char*, // forget
              const char*, // bump
              bool,        // backjump
              bool,        // minimize
              bool>       // phase_saving
SolverParams;

class SolverCoverageTest : public TestWithParam<SolverParams> {
protected:
  cdcl_solver solver;
  virtual void SetUp() {
    std::tie(solver.decide,
             solver.learn,
             solver.forget,
             solver.bump,
             solver.backjump,
             solver.minimize,
             solver.phase_saving)
      = GetParam();
  }
};

TEST_P(SolverCoverageTest, CT2) {
  istringstream s("p cnf 2 4\n1 2 0\n1 -2 0\n-1 2 0\n-1 -2 0\n");
  cnf f = parse_dimacs(s);
  proof pi = solver.solve(f);
  EXPECT_EQ(pi.resolution.size(), 2);
}

INSTANTIATE_TEST_CASE_P(ParametersTest,
                        SolverCoverageTest,
                        Combine(Values("fixed"),
                                Values("1uip","1uip-all","lastuip","decision"),
                                Values("nothing","everything","wide"),
                                Values("learnt","conflict"),
                                Values(true),
                                Bool(),
                                Bool()));

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

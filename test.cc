#include "data_structures.h"
#include "solver.h"
#include "dimacs.h"

#include <gtest/gtest.h>

using namespace std;

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

class SolverTest : public testing::Test {
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
  proof pi = solver.solve(f);
  EXPECT_EQ(pi.resolution.size(), 0);
}

TEST_F(SolverTest, contradiction) {
  istringstream s("p cnf 0 1\n0\n");
  cnf f = parse_dimacs(s);
  proof pi = solver.solve(f);
  EXPECT_EQ(pi.resolution.size(), 0);
}

TEST_F(SolverTest, sat) {
  istringstream s("p cnf 1 1\n1 0\n");
  cnf f = parse_dimacs(s);
  proof pi = solver.solve(f);
  EXPECT_EQ(pi.resolution.size(), 0);
}

TEST_F(SolverTest, unit) {
  istringstream s("p cnf 1 2\n1 0\n-1 0\n");
  cnf f = parse_dimacs(s);
  proof pi = solver.solve(f);
  EXPECT_EQ(pi.resolution.size(), 1);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

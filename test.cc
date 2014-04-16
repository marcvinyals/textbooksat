#include "data_structures.h"
#include "solver.h"

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
    solver.backjump = true;
    solver.minimize = false;
    solver.phase_saving = false;
  }
};

TEST_F(SolverTest, empty) {
  cnf f;
  f.variables=0;
  proof pi = solver.solve(f);
  EXPECT_EQ(pi.proof.size(), 0);
}

TEST_F(SolverTest, contradiction) {
  cnf f;
  f.clauses=vector<clause>({0});
  f.variables=0;
  proof pi = solver.solve(f);
  EXPECT_EQ(pi.proof.size(), 0);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

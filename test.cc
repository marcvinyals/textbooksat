#include "data_structures.h"

#include <gtest/gtest.h>

using namespace std;

class ClauseTest : public testing::Test {};
  
TEST_F(ClauseTest, subsumes) {
  clause c;
  clause d({vector<literal>({from_dimacs(1)})});
  EXPECT_TRUE(c.subsumes(d));
  EXPECT_FALSE(d.subsumes(c));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

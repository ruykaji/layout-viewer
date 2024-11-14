#include <gtest/gtest.h>

#include "Include/Utils.hpp"

TEST(UtilsTest, MakeRectangle)
{
  types::Polygon rect = utils::make_clockwise_rectangle({ 10, 5, -1, -4 });
  types::Polygon expected_result{ -1, 5, 10, 5, 10, -4, -1, -4 };

  EXPECT_EQ(rect, expected_result);
}

int
main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

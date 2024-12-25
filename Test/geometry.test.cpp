#include <gtest/gtest.h>

#include "Include/Geometry.hpp"

TEST(UtilsTest, MakeRectangle)
{
  geom::Polygon            polygon({ 10, 5, -1, -4 });
  std::vector<geom::Point> expected_points{ geom::Point{ -1, 5 }, geom::Point{ 10, 5 }, geom::Point{ 10, -4 }, geom::Point{ -1, -4 } };

  EXPECT_EQ(polygon.m_points, expected_points);
}

int
main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

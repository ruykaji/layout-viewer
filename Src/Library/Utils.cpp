#include "Include/Utils.hpp"

namespace utils
{

types::Rectangle
make_clockwise_rectangle(const std::array<int32_t, 4> vertices)
{
  int32_t min_x = std::min(vertices[0], vertices[2]);
  int32_t max_x = std::max(vertices[0], vertices[2]);
  int32_t min_y = std::min(vertices[1], vertices[3]);
  int32_t max_y = std::max(vertices[1], vertices[3]);

  return { min_x, max_y, max_x, max_y, max_x, min_y, min_x, min_y };
}

} // namespace utils

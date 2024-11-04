#include <algorithm>

#include "Include/Utils.hpp"

namespace utils
{

types::Rectangle
make_clockwise_rectangle(const std::array<double, 4> vertices)
{
  double min_x = std::min(vertices[0], vertices[2]);
  double max_x = std::max(vertices[0], vertices[2]);
  double min_y = std::min(vertices[1], vertices[3]);
  double max_y = std::max(vertices[1], vertices[3]);

  return { min_x, min_y, max_x, min_y, max_x, max_y, min_x, max_y };
}

types::Metal
get_skywater130_metal(const std::string_view str)
{
  if(str == "li1")
    {
      return types::Metal::L1;
    }

  if(str == "mcon")
    {
      return types::Metal::L1M1_V;
    }

  if(str == "met1")
    {
      return types::Metal::M1;
    }

  if(str == "via1")
    {
      return types::Metal::M1M2_V;
    }

  if(str == "met2")
    {
      return types::Metal::M2;
    }

  if(str == "via2")
    {
      return types::Metal::M2M3_V;
    }

  if(str == "met3")
    {
      return types::Metal::M3;
    }

  if(str == "via3")
    {
      return types::Metal::M3M4_V;
    }

  if(str == "met4")
    {
      return types::Metal::M4;
    }

  if(str == "via4")
    {
      return types::Metal::M4M5_V;
    }

  if(str == "met5")
    {
      return types::Metal::M5;
    }

  return types::Metal::NONE;
}

bool
are_rectangle_intersects(const types::Rectangle& lhs_rect, const types::Rectangle& rhs_rect)
{
  if(lhs_rect[0] > rhs_rect[4] || rhs_rect[0] > lhs_rect[4])
    {
      return false;
    }

  if(lhs_rect[1] > rhs_rect[5] || rhs_rect[1] > lhs_rect[5])
    {
      return false;
    }

  return true;
}

types::Rectangle
get_rect_overlap(const types::Rectangle& lhs_rect, const types::Rectangle& rhs_rect)
{
  double left   = std::max(lhs_rect[0], rhs_rect[0]);
  double top    = std::max(lhs_rect[1], rhs_rect[1]);

  double right  = std::min(lhs_rect[4], rhs_rect[4]);
  double bottom = std::min(lhs_rect[5], rhs_rect[5]);

  return { left, top, right, top, right, bottom, left, bottom };
}

} // namespace utils

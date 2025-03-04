#include "Include/DEF/GCell.hpp"

namespace def
{

std::vector<std::pair<GCell*, geom::Polygon>>
GCell::find_overlaps(const geom::Polygon& poly, const std::vector<std::vector<GCell*>>& gcells, const uint32_t width, const uint32_t height)
{
  const std::size_t num_rows          = gcells.size();
  const std::size_t num_cols          = gcells[0].size();

  const std::size_t step_row          = height / num_rows;
  const std::size_t step_col          = width / num_cols;

  const auto [left_top, right_bottom] = poly.get_extrem_points();

  std::size_t left_most_gcell         = std::clamp<std::size_t>(std::floor(left_top.x / step_col), 0, num_cols - 1);
  std::size_t top_most_gcell          = std::clamp<std::size_t>(std::floor(left_top.y / step_row), 0, num_rows - 1);
  std::size_t right_most_gcell        = std::clamp<std::size_t>(std::floor((right_bottom.x - 1) / step_col), 0, num_cols - 1);
  std::size_t bottom_most_gcell       = std::clamp<std::size_t>(std::floor((right_bottom.y - 1) / step_row), 0, num_rows - 1);

  auto        adjust_gcell            = [&](std::size_t& gcell_row, std::size_t& gcell_col, const geom::Point& target_point) -> bool {
    if(gcell_row >= num_rows || gcell_col >= num_cols)
      {
        return false;
      }

    const auto& box_points = gcells[gcell_row][gcell_col]->m_box.m_points;

    if((box_points[1].x <= target_point.x && box_points[0].x >= target_point.x && box_points[2].y <= target_point.y && box_points[0].y >= target_point.y))
      {
        return false;
      }

    if(box_points[1].x > target_point.x && gcell_col > 0)
      {
        --gcell_col;
      }
    else if(box_points[0].x < target_point.x && gcell_col < num_cols - 1)
      {
        ++gcell_col;
      }

    if(box_points[2].y > target_point.y && gcell_row > 0)
      {
        --gcell_row;
      }
    else if(box_points[0].y < target_point.y && gcell_row < num_rows - 1)
      {
        ++gcell_row;
      }
    else
      {
        return false;
      }

    return true;
  };

  while(true)
    {
      if(!adjust_gcell(top_most_gcell, left_most_gcell, left_top))
        {
          break;
        }
    }

  while(true)
    {
      if(!adjust_gcell(bottom_most_gcell, right_most_gcell, right_bottom))
        {
          break;
        }
    }

  std::vector<std::pair<GCell*, geom::Polygon>> gcells_with_overlap;

  for(std::size_t y = top_most_gcell; y <= std::min(num_rows - 1, bottom_most_gcell); ++y)
    {
      for(std::size_t x = left_most_gcell; x <= std::min(num_cols - 1, right_most_gcell); ++x)
        {
          if(poly / gcells[y][x]->m_box)
            {
              gcells_with_overlap.emplace_back(gcells[y][x], std::move(poly - gcells[y][x]->m_box));
            }
        }
    }

  return gcells_with_overlap;
}

} // namespace def
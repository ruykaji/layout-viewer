#ifndef __DEF_UTILS_HPP__
#define __DEF_UTILS_HPP__

#include <map>

#include "Include/Geometry.hpp"

namespace def::utils
{

struct AccessPoints
{
  types::Metal                                      m_metal = types::Metal::NONE;
  std::vector<std::pair<geom::Point, geom::PointS>> m_points;
};

struct MetalGrid
{
  geom::Point m_start;
  geom::Point m_end;
  double      m_step;

  struct Compare
  {
    bool
    operator()(const types::Metal& lhs, const types::Metal& rhs) const
    {
      return uint8_t(lhs) < uint8_t(rhs);
    }
  };
};

template <typename Tp>
auto
project(const geom::Point& point, const MetalGrid& grid) noexcept(true)
{
  double proj_x = std::round((point.x - grid.m_start.x) / grid.m_step) * grid.m_step + grid.m_start.x;
  proj_x        = std::max(proj_x, grid.m_start.x);
  proj_x        = std::min(proj_x, grid.m_end.x);

  double proj_y = std::round((point.y - grid.m_start.y) / grid.m_step) * grid.m_step + grid.m_start.y;
  proj_y        = std::max(proj_y, grid.m_start.y);
  proj_y        = std::min(proj_y, grid.m_end.y);

  if constexpr(std::is_same_v<Tp, geom::Point>)
    {
      const geom::Point proj = { proj_x, proj_y };
      return proj;
    }

  if constexpr(std::is_same_v<Tp, geom::PointS>)
    {
      const geom::PointS proj_int = { (proj_x - grid.m_start.x) / grid.m_step, (proj_y - grid.m_start.y) / grid.m_step };
      return proj_int;
    }
}

template <typename Tp>
auto
project_down(const geom::Point& point, const types::Metal source, const std::map<types::Metal, MetalGrid, MetalGrid::Compare>& grids) noexcept(true)
{
  double proj_x = point.x;
  double proj_y = point.y;

  for(auto itr = grids.find(source), end = grids.end(); itr != end; --itr)
    {
      const MetalGrid& grid = itr->second;

      proj_x                = std::round((proj_x - grid.m_start.x) / grid.m_step) * grid.m_step + grid.m_start.x;
      proj_x                = std::max(proj_x, grid.m_start.x);
      proj_x                = std::min(proj_x, grid.m_end.x);

      proj_y                = std::round((proj_y - grid.m_start.y) / grid.m_step) * grid.m_step + grid.m_start.y;
      proj_y                = std::max(proj_y, grid.m_start.y);
      proj_y                = std::min(proj_y, grid.m_end.y);
    }

  if constexpr(std::is_same_v<Tp, geom::Point>)
    {
      const geom::Point proj = { proj_x, proj_y };
      return proj;
    }

  if constexpr(std::is_same_v<Tp, geom::PointS>)
    {
      const MetalGrid&   grid     = grids.begin()->second;
      const geom::PointS proj_int = { (proj_x - grid.m_start.x) / grid.m_step, (proj_y - grid.m_start.y) / grid.m_step };
      return proj_int;
    }
}

std::vector<std::pair<geom::Point, geom::PointS>>
find_access_points(const geom::Polygon& poly, const types::Metal target, const std::map<types::Metal, MetalGrid, MetalGrid::Compare>& grids);

} // namespace def::utils

#endif
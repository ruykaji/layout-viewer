#include "Include/DEF/Utils.hpp"

namespace def::utils
{

std::vector<std::pair<geom::Point, geom::PointS>>
find_access_points(const geom::Polygon& poly, const types::Metal target, const std::map<types::Metal, MetalGrid, MetalGrid::Compare>& grids)
{
  /** First find all access points within poly metal layer */
  const auto [left_top, right_bottom]                            = poly.get_extrem_points();

  const types::Metal                                target_metal = target == types::Metal::L1 ? types::Metal::M1 : target;
  const MetalGrid&                                  grid         = grids.at(target_metal);

  const geom::Point                                 grid_start   = project<geom::Point>(left_top, grid);
  const geom::Point                                 grid_end     = project<geom::Point>(right_bottom, grid);

  std::vector<std::pair<geom::Point, geom::PointS>> inner_access_points;
  std::vector<std::pair<geom::Point, geom::PointS>> outer_access_points;

  for(double y = grid_start.y; y <= grid_end.y; y += grid.m_step)
    {
      for(double x = grid_start.x; x <= grid_end.x; x += grid.m_step)
        {
          const geom::Point  point = { x, y };
          const geom::PointS proj  = project_down<geom::PointS>(point, target_metal, grids);

          if(poly.probe_point(point))
            {
              inner_access_points.emplace_back(point, proj);
            }
          else
            {
              outer_access_points.emplace_back(point, proj);
            }
        }
    }

  if(inner_access_points.empty() && outer_access_points.empty())
    {
      throw std::runtime_error("Unable to find any access points of a polygon");
    }

  return inner_access_points.empty() ? outer_access_points : inner_access_points;
}

} // namespace def::utils

#include <array>

#include <clipper2/clipper.h>

#include "Include/Geometry.hpp"

namespace geom
{

namespace details
{

bool
do_edges_intersect(const Point& p1, const Point& q1, const Point& p2, const Point& q2)
{
  auto orientation = [](const Point& p, const Point& q, const Point& r) {
    double val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);

    if(std::fabs(val) < 1e-4)
      return 0;

    return (val > 0) ? 1 : 2;
  };

  int o1 = orientation(p1, q1, p2);
  int o2 = orientation(p1, q1, q2);
  int o3 = orientation(p2, q2, p1);
  int o4 = orientation(p2, q2, q1);

  if(o1 != o2 && o3 != o4)
    return true;

  auto onSegment = [](const Point& p, const Point& q, const Point& r) {
    return q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) && q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y);
  };

  if(o1 == 0 && onSegment(p1, p2, q1))
    return true;
  if(o2 == 0 && onSegment(p1, q2, q1))
    return true;
  if(o3 == 0 && onSegment(p2, p1, q2))
    return true;
  if(o4 == 0 && onSegment(p2, q1, q2))
    return true;

  return false;
}

} // namespace details

Polygon::Polygon()
    : m_metal(types::Metal::NONE), m_left_top(0, 0), m_right_bottom(0, 0), m_center(0, 0), m_points() {};

Polygon::Polygon(const std::array<double, 4> ver, types::Metal metal)
    : m_metal(metal)
{
  double min_x = std::min(ver[0], ver[2]);
  double max_x = std::max(ver[0], ver[2]);
  double min_y = std::min(ver[1], ver[3]);
  double max_y = std::max(ver[1], ver[3]);

  m_points     = { Point{ max_x, max_y }, Point{ min_x, max_y }, Point{ min_x, min_y }, Point{ max_x, min_y } };

  update_extrem_points();
  update_center();
}

Polygon
operator+(const Polygon& lhs, const Polygon& rhs)
{
  if(lhs.m_metal != rhs.m_metal)
    {
      throw std::runtime_error("Geometry Error: It is impossible to union polygons between different layers.");
    }

  if(rhs.m_points.empty())
    {
      return lhs;
    }

  if(lhs.m_points.empty())
    {
      return rhs;
    }

  std::vector<std::vector<Point>> solution = Clipper2Lib::Union({ lhs.m_points }, { rhs.m_points }, Clipper2Lib::FillRule::NonZero, 8);

  if(solution.empty())
    {
      return {};
    }

  Polygon polygon;
  polygon.m_metal  = lhs.m_metal;
  polygon.m_points = solution[0];
  polygon.update_extrem_points();
  polygon.update_center();

  return std::move(polygon);
}

Polygon
operator-(const Polygon& lhs, const Polygon& rhs)
{
  if(lhs.m_metal != rhs.m_metal && rhs.m_metal != types::Metal::NONE)
    {
      throw std::runtime_error("Geometry Error: It is impossible to intersect polygons between different layers.");
    }

  if(rhs.m_points.empty())
    {
      return lhs;
    }

  if(lhs.m_points.empty())
    {
      return rhs;
    }

  std::vector<std::vector<Point>> solution = Clipper2Lib::Intersect({ lhs.m_points }, { rhs.m_points }, Clipper2Lib::FillRule::NonZero, 8);

  if(solution.empty())
    {
      return {};
    }

  Polygon polygon;
  polygon.m_metal  = lhs.m_metal;
  polygon.m_points = solution[0];
  polygon.update_extrem_points();
  polygon.update_center();

  return std::move(polygon);
}

bool
operator/(const Polygon& lhs, const Polygon& rhs)
{
  if(lhs.m_points.empty() || rhs.m_points.empty())
    {
      return false;
    }

  std::vector<std::vector<Point>> solution = Clipper2Lib::Intersect({ lhs.m_points }, { rhs.m_points }, Clipper2Lib::FillRule::NonZero, 8);

  if(!solution.empty())
    {
      return true;
    }

  for(size_t i = 0; i < lhs.m_points.size(); ++i)
    {
      Point p1 = lhs.m_points[i];
      Point q1 = lhs.m_points[(i + 1) % lhs.m_points.size()];

      for(size_t j = 0; j < rhs.m_points.size(); ++j)
        {
          Point p2 = rhs.m_points[j];
          Point q2 = rhs.m_points[(j + 1) % rhs.m_points.size()];

          if(details::do_edges_intersect(p1, q1, p2, q2))
            {
              return true;
            }
        }
    }

  return false;
}

Polygon&
Polygon::operator+=(const Polygon& rhs)
{
  if(m_metal != rhs.m_metal)
    {
      throw std::runtime_error("Geometry Error: It is impossible to union polygons between different layers.");
    }

  if(rhs.m_points.empty())
    {
      return *this;
    }

  std::vector<std::vector<Point>> solution = Clipper2Lib::Union({ m_points }, { rhs.m_points }, Clipper2Lib::FillRule::NonZero, 8);

  m_points                                 = std::move(solution[0]);
  update_extrem_points();
  update_center();

  return *this;
}

Polygon&
Polygon::operator-(const Polygon& rhs)
{
  if(m_metal != rhs.m_metal && rhs.m_metal != types::Metal::NONE)
    {
      throw std::runtime_error("Geometry Error: It is impossible to intersect polygons between different layers.");
    }

  if(rhs.m_points.empty())
    {
      return *this;
    }

  std::vector<std::vector<Point>> solution = Clipper2Lib::Intersect({ m_points }, { rhs.m_points }, Clipper2Lib::FillRule::NonZero, 8);

  m_points                                 = std::move(solution[0]);
  update_extrem_points();
  update_center();

  return *this;
}

void
Polygon::scale_by(const double factor)
{
  for(auto& point : m_points)
    {
      point.x *= factor;
      point.y *= factor;
    }

  // TODO Optimize this calls
  update_extrem_points();
  update_center();
}

void
Polygon::move_by(const Point offset)
{
  for(auto& point : m_points)
    {
      point.x += offset.x;
      point.y += offset.y;
    }

  // TODO Optimize this calls
  update_extrem_points();
  update_center();
}

void
Polygon::update_extrem_points()
{
  m_left_top     = m_points[0];
  m_right_bottom = m_points[0];

  for(const auto& point : m_points)
    {
      if(point.x > m_right_bottom.x || (point.x == m_right_bottom.x && point.y > m_right_bottom.y))
        {
          m_right_bottom = point;
        }

      if(point.x < m_left_top.x || (point.x == m_left_top.x && point.y < m_left_top.y))
        {
          m_left_top = point;
        }
    }
}

void
Polygon::update_center()
{
}

} // namespace geom

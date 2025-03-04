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
      {
        return 0;
      }

    return (val > 0) ? 1 : 2;
  };

  int32_t o1 = orientation(p1, q1, p2);
  int32_t o2 = orientation(p1, q1, q2);
  int32_t o3 = orientation(p2, q2, p1);
  int32_t o4 = orientation(p2, q2, q1);

  if(o1 != o2 && o3 != o4)
    {
      return true;
    }

  auto on_segment = [](const Point& p, const Point& q, const Point& r) {
    return q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) && q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y);
  };

  if(o1 == 0 && on_segment(p1, p2, q1))
    {
      return true;
    }
  else if(o2 == 0 && on_segment(p1, q2, q1))
    {
      return true;
    }
  else if(o3 == 0 && on_segment(p2, p1, q2))
    {
      return true;
    }
  else if(o4 == 0 && on_segment(p2, q1, q2))
    {
      return true;
    }

  return false;
}

} // namespace details

Polygon::Polygon()
    : m_metal(types::Metal::NONE), m_points() {};

Polygon::Polygon(const std::array<double, 4> ver, types::Metal metal)
    : m_metal(metal)
{
  double min_x = std::min(ver[0], ver[2]);
  double max_x = std::max(ver[0], ver[2]);
  double min_y = std::min(ver[1], ver[3]);
  double max_y = std::max(ver[1], ver[3]);

  m_points     = { Point{ max_x, max_y }, Point{ min_x, max_y }, Point{ min_x, min_y }, Point{ max_x, min_y } };
}

Polygon
operator+(const Polygon& lhs, const Polygon& rhs)
{
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

  return std::move(polygon);
}

Polygon
operator-(const Polygon& lhs, const Polygon& rhs)
{
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

  return std::move(polygon);
}

bool
operator/(const Polygon& lhs, const Polygon& rhs)
{
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
  if(rhs.m_points.empty())
    {
      return *this;
    }

  std::vector<std::vector<Point>> solution = Clipper2Lib::Union({ m_points }, { rhs.m_points }, Clipper2Lib::FillRule::NonZero, 8);

  m_points                                 = std::move(solution[0]);

  return *this;
}

Polygon&
Polygon::operator-=(const Polygon& rhs)
{
  if(rhs.m_points.empty())
    {
      return *this;
    }

  std::vector<std::vector<Point>> solution = Clipper2Lib::Intersect({ m_points }, { rhs.m_points }, Clipper2Lib::FillRule::NonZero, 8);

  m_points                                 = std::move(solution[0]);

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
}

void
Polygon::move_by(const Point offset)
{
  for(auto& point : m_points)
    {
      point.x += offset.x;
      point.y += offset.y;
    }
}

std::pair<Point, Point>
Polygon::get_extrem_points() const
{
  Point left_top     = m_points[0];
  Point right_bottom = m_points[0];

  for(const auto& point : m_points)
    {
      if(point.x > right_bottom.x)
        {
          right_bottom.x = point.x;
        }

      if(point.x < left_top.x)
        {
          left_top.x = point.x;
        }

      if(point.y > right_bottom.y)
        {
          right_bottom.y = point.y;
        }

      if(point.y < left_top.y)
        {
          left_top.y = point.y;
        }
    }

  return std::make_pair(left_top, right_bottom);
}

Point
Polygon::get_center() const
{
  // double            signed_area = 0.0;
  // Point             center{ 0.0, 0.0 };

  // const std::size_t n = m_points.size();

  // for(std::size_t i = 0; i < n; ++i)
  //   {
  //     const Point& p1 = m_points[i];
  //     const Point& p2 = m_points[(i + 1) % n];

  //     double       a  = p1.x * p2.y - p2.x * p1.y;

  //     signed_area += a;
  //     center.x += (p1.x + p2.x) * a;
  //     center.y += (p1.y + p2.y) * a;
  //   }

  // signed_area *= 0.5;
  // center.x /= (6.0 * signed_area);
  // center.y /= (6.0 * signed_area);

  const auto [left_top, right_bottom] = get_extrem_points();
  return { (left_top.x + right_bottom.x) / 2.0, (left_top.y + right_bottom.y) / 2.0 };
}

bool
Polygon::probe_point(const Point& point) const
{
  return Clipper2Lib::PointInPolygon(point, m_points) != Clipper2Lib::PointInPolygonResult::IsOutside;
}

double
Polygon::get_area() const
{
  return Clipper2Lib::Area<double>(m_points);
}

} // namespace geom

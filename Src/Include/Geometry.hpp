#ifndef __GEOMETRY_HPP__
#define __GEOMETRY_HPP__

#include <array>

#include <clipper2/clipper.h>

#include "Types.hpp"

namespace geom
{

using PointS = Clipper2Lib::Point<std::size_t>;

struct LineS
{
  PointS       m_start;
  PointS       m_end;
  types::Metal m_metal;
};

using Point = Clipper2Lib::Point<double>;

struct Line
{
  Point        m_start;
  Point        m_end;
  types::Metal m_metal;
};

class Polygon
{
public:
  Polygon();

  Polygon(const std::array<double, 4> ver, types::Metal metal = types::Metal::NONE);

public:
  /**
   * @brief Union of two polygons.
   *
   * @param lhs The first polygon.
   * @param rhs The second polygon.
   * @return Polygon
   */
  friend Polygon
  operator+(const Polygon& lhs, const Polygon& rhs);

  /**
   * @brief Intersection of two polygons.
   *
   * @param lhs The first polygon.
   * @param rhs The second polygon.
   * @return Polygon
   */
  friend Polygon
  operator-(const Polygon& lhs, const Polygon& rhs);

  /**
   * @brief Checks if two polygons are intersects.
   *
   * @param lhs The first polygon.
   * @param rhs The second polygon.
   * @return true
   * @return false
   */
  friend bool
  operator/(const Polygon& lhs, const Polygon& rhs);

  /**
   * @brief In place union with another polygon.
   *
   * @param rhs The polygon.
   * @return Polygon&
   */
  Polygon&
  operator+=(const Polygon& rhs);

  /**
   * @brief In place intersection with another polygon.
   *
   * @param rhs The polygon.
   * @return Polygon&
   */
  Polygon&
  operator-=(const Polygon& rhs);

public:
  /**
   * @brief Scale the polygon by the factor.
   *
   * @param factor
   */
  void
  scale_by(const double factor);

  /**
   * @brief Move the polygon the offset.
   *
   * @param offset
   */
  void
  move_by(const Point offset);

  /**
   * @brief Gets most top left and most bottom right points of the polygon.
   *
   */
  std::pair<Point, Point>
  get_extrem_points() const;

  /**
   * @brief Gets center of the polygon.
   *
   */
  Point
  get_center() const;

  /**
   * @brief Checks if point lies in polygon.
   *
   * @param point The point to check.
   * @return true
   * @return false
   */
  bool
  probe_point(const Point& point) const;

  /**
   * @brief Gets the area of a polygon.
   *
   * @return double
   */
  double
  get_area() const;

public:
  types::Metal       m_metal;
  std::vector<Point> m_points;
};

} // namespace geom

#endif
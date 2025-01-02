#ifndef __GEOMETRY_HPP__
#define __GEOMETRY_HPP__

#include <array>

#include <clipper2/clipper.h>

#include "Types.hpp"

namespace geom
{

using Point = Clipper2Lib::Point<double>;

struct Polygon
{
  types::Metal       m_metal;
  std::vector<Point> m_points;

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
  operator-(const Polygon& rhs);

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
};

} // namespace geom

#endif
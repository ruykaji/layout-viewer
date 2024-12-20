#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <string_view>

#include <clipper2/clipper.h>

#include "Include/Types.hpp"

namespace utils
{

/**
 * @brief Constructs a rectangle in clockwise order from given vertices.
 *
 * This function takes an array of four integers representing two points
 * (x0, y0) and (x1, y1). It calculates the minimum and maximum x and y
 * coordinates to define the rectangle's corners. The rectangle is then
 * returned with its vertices arranged in clockwise order starting from
 * the top-left corner.
 *
 * @param vertices An array containing four integers: {x0, y0, x1, y1}.
 * @return Clipper2Lib::PathD
 */
Clipper2Lib::PathD
make_clockwise_rectangle(const std::array<double, 4> vertices);

/**
 * @brief Gets metal type according to SkyWater130 library's naming of metal layers.
 *
 * @param str Metal layer as string.
 * @return types::Metal
 */
types::Metal
get_skywater130_metal(const std::string_view str);

bool
are_rectangle_intersects(const types::Polygon& lhs_rect, const types::Polygon& rhs_rect);

types::Polygon
get_rect_overlap(const types::Polygon& lhs_rect, const types::Polygon& rhs_rect);

} // namespace utils

#endif
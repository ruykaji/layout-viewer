#ifndef __UTILS_HPP__
#define __UTILS_HPP__

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
 * @return A `types::Rectangle` object representing the rectangle.
 */
types::Rectangle
make_clockwise_rectangle(const std::array<int32_t, 4> vertices);

} // namespace utils

#endif
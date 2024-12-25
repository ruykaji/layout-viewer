#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <string_view>

#include "Include/Types.hpp"

namespace utils
{

/**
 * @brief Gets metal type according to SkyWater130 library's naming of metal layers.
 *
 * @param str Metal layer as string.
 * @return types::Metal
 */
types::Metal
get_skywater130_metal(const std::string_view str);

} // namespace utils

#endif
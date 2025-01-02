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

std::string
skywater130_metal_to_string(const types::Metal metal);

std::tuple<uint32_t, uint32_t, uint32_t>
get_metal_color(types::Metal metal);

std::string
get_color_from_string(const std::string& string);

} // namespace utils

#endif
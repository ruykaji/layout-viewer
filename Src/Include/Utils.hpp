#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include "Include/Types.hpp"

namespace utils
{

types::Rectangle
make_clockwise_rectangle(const std::array<int32_t, 4> vertices);

} // namespace utils

#endif
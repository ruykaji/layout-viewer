#ifndef __TYPES_HPP__
#define __TYPES_HPP__

#include <array>
#include <cstdint>

namespace types
{

enum class Metal
{
  NONE = 0,
  L1,
  L1M1_V,
  M1,
  M1M2_V,
  M2,
  M2M3_V,
  M3,
  M3M4_V,
  M4,
  M4M5_V,
  M5,
  M5M6_V,
  M6,
  M6M7_V,
  M7,
  M7M8_V,
  M8,
  M8M9_V,
  M9
};

using Rectangle = std::array<int32_t, 8>;

} // namespace types

#endif
#ifndef __TYPES_HPP__
#define __TYPES_HPP__

#include <cstdint>
#include <vector>

namespace types
{

enum class Orientation
{
  N = 0, /** 0 */
  E,     /** 90 */
  S,     /** 180 */
  W,     /** 270 */
  FN,    /** mirrored horizontally */
  FE,    /** 90 with mirroring */
  FS,    /** 180 with mirroring */
  FW     /** 270 with mirroring */
};

enum class Metal : uint8_t
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
  M9,
  SIZE = 19
};

enum class Cell : uint8_t
{
  TRACE           = 1,
  TRACE_INNER_VIA = 2,

  TERMINAL        = 20,
  // INNER_VIA = 125,
  PATH            = 10,
  // OUTER_VIA = 255,
};

} // namespace types

#endif
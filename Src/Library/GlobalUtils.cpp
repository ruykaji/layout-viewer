#include <algorithm>
#include <cmath>

#include "Include/GlobalUtils.hpp"

namespace utils
{

types::Metal
get_skywater130_metal(const std::string_view str)
{
  if(str == "li1")
    {
      return types::Metal::L1;
    }

  if(str == "mcon")
    {
      return types::Metal::L1M1_V;
    }

  if(str == "met1")
    {
      return types::Metal::M1;
    }

  if(str == "via1")
    {
      return types::Metal::M1M2_V;
    }

  if(str == "met2")
    {
      return types::Metal::M2;
    }

  if(str == "via2")
    {
      return types::Metal::M2M3_V;
    }

  if(str == "met3")
    {
      return types::Metal::M3;
    }

  if(str == "via3")
    {
      return types::Metal::M3M4_V;
    }

  if(str == "met4")
    {
      return types::Metal::M4;
    }

  if(str == "via4")
    {
      return types::Metal::M4M5_V;
    }

  if(str == "met5")
    {
      return types::Metal::M5;
    }

  return types::Metal::NONE;
}

std::string
skywater130_metal_to_string(const types::Metal metal)
{
  switch(metal)
    {
    case types::Metal::L1:
      {
        return "li1";
      }
    case types::Metal::L1M1_V:
      {
        return "mcon";
      }
    case types::Metal::M1:
      {
        return "met1";
      }
    case types::Metal::M1M2_V:
      {
        return "via1";
      }
    case types::Metal::M2:
      {
        return "met2";
      }
    case types::Metal::M2M3_V:
      {
        return "via2";
      }
    case types::Metal::M3:
      {
        return "met3";
      }
    case types::Metal::M3M4_V:
      {
        return "via3";
      }
    case types::Metal::M4:
      {
        return "met4";
      }
    case types::Metal::M4M5_V:
      {
        return "via4";
      }
    case types::Metal::M5:
      {
        return "met5";
      }
    default: break;
    }

  return "NONE";
}

std::tuple<uint32_t, uint32_t, uint32_t>
get_metal_color(types::Metal metal)
{
  switch(metal)
    {
    case types::Metal::L1:
      {
        return std::make_tuple(0, 0, 255);
      }
    case types::Metal::L1M1_V:
      {
        return std::make_tuple(255, 0, 0);
      }
    case types::Metal::M1:
      {
        return std::make_tuple(255, 0, 0);
      }
    case types::Metal::M2:
      {
        return std::make_tuple(0, 255, 0);
      }
    case types::Metal::M3:
      {
        return std::make_tuple(255, 255, 0);
      }
    case types::Metal::M4:
      {
        return std::make_tuple(0, 255, 255);
      }
    case types::Metal::M5:
      {
        return std::make_tuple(255, 0, 255);
      }
    case types::Metal::M6:
      {
        return std::make_tuple(125, 125, 255);
      }
    case types::Metal::M7:
      {
        return std::make_tuple(255, 125, 125);
      }
    case types::Metal::M8:
      {
        return std::make_tuple(125, 255, 125);
      }
    case types::Metal::M9:
      {
        return std::make_tuple(255, 75, 125);
      }
    default:
      {
        return std::make_tuple(255, 255, 255);
      }
    }
}

std::string
get_color_from_string(const std::string& string)
{
  const std::hash<std::string> hasher;
  const std::size_t            hash_value = (hasher(string) * 2654435761) ^ 0xDEADBEEF;

  unsigned char                red        = (hash_value & 0xFF0000) >> 16;
  unsigned char                green      = (hash_value & 0x00FF00) >> 8;
  unsigned char                blue       = (hash_value & 0x0000FF);

  char                         color[8];
  snprintf(color, sizeof(color), "#%02X%02X%02X", red, green, blue);

  return color;
}

} // namespace utils

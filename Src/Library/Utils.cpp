#include <algorithm>
#include <cmath>

#include "Include/Utils.hpp"

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

} // namespace utils

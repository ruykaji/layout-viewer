#ifndef __GUIDE_HPP__
#define __GUIDE_HPP__

#include <filesystem>
#include <string>
#include <vector>

#include "Include/Geometry.hpp"

namespace guide
{

struct Net
{
  std::string                m_name;
  std::vector<geom::Polygon> m_path;
};

std::vector<Net>
read(const std::filesystem::path& path);

} // namespace

#endif
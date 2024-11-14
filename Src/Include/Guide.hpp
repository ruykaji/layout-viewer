#ifndef __GUIDE_HPP__
#define __GUIDE_HPP__

#include <filesystem>
#include <string>
#include <vector>

#include "Include/Types.hpp"

namespace guide
{

struct Net
{
  std::string                                          m_name;
  std::vector<std::pair<types::Polygon, types::Metal>> m_path;
};

std::vector<Net>
read(const std::filesystem::path& path);

} // namespace

#endif
#include <fstream>
#include <iostream>

#include "Include/Guide.hpp"
#include "Include/Utils.hpp"

namespace guide
{

std::vector<Net>
read(const std::filesystem::path& path)
{
  if(!std::filesystem::exists(path) || path.extension() != ".guide")
    {
      throw std::runtime_error("Guide Error: Can't locate guide file by the path - \"" + path.string() + "\".");
    }

  std::ifstream in_file(path);

  if(!in_file.is_open() || !in_file.good())
    {
      throw std::runtime_error("Guide Error: Can't open the file - \"" + path.string() + "\".");
    }

  std::string      line;
  std::vector<Net> nets;

  while(std::getline(in_file, line))
    {
      if(nets.empty())
        {
          nets.emplace_back(line);
          continue;
        }

      if(line == "(")
        {
          continue;
        }

      if(line == ")" && std::getline(in_file, line))
        {
          nets.emplace_back(line);
          continue;
        }

      if(line.empty())
        {
          break;
        }

      std::size_t pos      = 0;
      std::size_t next_pos = line.find(' ');
      double      x1       = std::stod(line.substr(pos, next_pos - pos));

      pos                  = next_pos + 1;
      next_pos             = line.find(' ', pos);
      double y1            = std::stod(line.substr(pos, next_pos - pos));

      pos                  = next_pos + 1;
      next_pos             = line.find(' ', pos);
      double x2            = std::stod(line.substr(pos, next_pos - pos));

      pos                  = next_pos + 1;
      next_pos             = line.find(' ', pos);
      double       y2      = std::stod(line.substr(pos, next_pos - pos));

      types::Metal metal   = utils::get_skywater130_metal(line.substr(next_pos + 1));

      nets.back().m_path.emplace_back(utils::make_clockwise_rectangle({ x1, y1, x2, y2 }), metal);
    }

  return nets;
}

} // namespace guide

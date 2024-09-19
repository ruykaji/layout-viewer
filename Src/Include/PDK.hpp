#ifndef __PDK_HPP__
#define __PDK_HPP__

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "LEF.hpp"
#include "Types.hpp"

#define GET_PDK()                                                                                                                          \
  if(data == nullptr)                                                                                                                      \
    {                                                                                                                                      \
      throw std::runtime_error("PDK Error: Loss of context.");                                                                             \
    }                                                                                                                                      \
  PDK* pdk = reinterpret_cast<PDK*>(data)

namespace pdk
{

struct Layer
{
  enum class Type
  {
    ROUTING = 0,
    CUT,
  };

  Type     m_type;
  uint32_t m_width;
  uint32_t m_pitch;
};

struct Pin
{
  enum class Type
  {
    SIGNAL = 0,
  };

  Type                          type;
  std::string                   m_name;
  std::vector<types::Rectangle> m_geometry;
};

struct Macro
{
  std::size_t                   m_width;
  std::size_t                   m_height;
  std::vector<Pin>              m_pins;
  std::vector<types::Rectangle> m_geometry;
};

struct PDK
{
  std::unordered_map<types::Metal, Layer> m_layers;
  std::unordered_map<std::string, Macro>  m_macros;
};

class Reader : public lef::LEF
{
public:
  Reader() : lef::LEF() {};

  PDK
  compress(const std::filesystem::path& dir_path) const
  {
    if(!std::filesystem::exists(dir_path))
      {
        throw std::invalid_argument("Can't find directory with pdk by path - \"" + dir_path.string() + "\".");
      }

    PDK                                pdk;

    std::vector<std::filesystem::path> lef_files;
    std::vector<std::filesystem::path> tech_lef_files;

    for(auto& itr : std::filesystem::recursive_directory_iterator(dir_path))
      {
        if(itr.is_regular_file())
          {
            const std::filesystem::path file_path      = itr.path();
            const std::string_view      file_path_view = file_path.c_str();
            const std::size_t           extension_pos  = file_path_view.find_first_of('.');

            if(extension_pos != std::string::npos)
              {
                const std::string_view extension = file_path_view.substr(extension_pos);

                if(extension == ".lef")
                  {
                    lef_files.emplace_back(std::move(file_path));
                  }
                else if(extension == ".tlef")
                  {
                    tech_lef_files.emplace_back(std::move(file_path));
                  }
              }
          }
      }

    for(const auto& path : tech_lef_files)
      {
        parse(path, reinterpret_cast<void*>(&pdk));
      }

    for(const auto& path : lef_files)
      {
        parse(path, reinterpret_cast<void*>(&pdk));
      }

    return pdk;
  }

  PDK
  load(const std::filesystem::path& file_path) const
  {
  }

protected:
  virtual void
  layer_callback(lefiLayer* param, void* data) const override
  {
    GET_PDK();
  }
};

} // namespace pdk

#endif
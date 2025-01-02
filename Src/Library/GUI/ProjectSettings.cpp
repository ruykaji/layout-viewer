#include <fstream>

#include "Include/GUI/ProjectSettings.hpp"

void
ProjectSettings::save_to(const std::filesystem::path& path) const
{
  if(std::filesystem::exists(path))
    {
      throw std::runtime_error("An exception has occurred while trying to save project settings.");
    }

  std::ofstream out_file(path);
  std::size_t   length;

  length = m_name.size();
  out_file.write(reinterpret_cast<const char*>(&length), sizeof(length));
  out_file.write(m_name.data(), length);

  length = m_pdk_folder.size();
  out_file.write(reinterpret_cast<const char*>(&length), sizeof(length));
  out_file.write(m_pdk_folder.data(), length);

  length = m_def_file.size();
  out_file.write(reinterpret_cast<const char*>(&length), sizeof(length));
  out_file.write(m_def_file.data(), length);

  length = m_guide_file.size();
  out_file.write(reinterpret_cast<const char*>(&length), sizeof(length));
  out_file.write(m_guide_file.data(), length);

  out_file.close();
}

void
ProjectSettings::read_from(const std::filesystem::path& path)
{
  if(!std::filesystem::exists(path))
    {
      throw std::runtime_error("An exception has occurred while trying to load project settings.");
    }

  std::ifstream in_file(path);
  std::size_t   length;

  in_file.read(reinterpret_cast<char*>(&length), sizeof(length));
  m_name.resize(length);
  in_file.read(&m_name[0], length);

  in_file.read(reinterpret_cast<char*>(&length), sizeof(length));
  m_pdk_folder.resize(length);
  in_file.read(&m_pdk_folder[0], length);

  in_file.read(reinterpret_cast<char*>(&length), sizeof(length));
  m_def_file.resize(length);
  in_file.read(&m_def_file[0], length);
  in_file.read(reinterpret_cast<char*>(&length), sizeof(length));

  m_guide_file.resize(length);
  in_file.read(&m_guide_file[0], length);
}

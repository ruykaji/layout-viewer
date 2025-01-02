#ifndef __PROJECT_SETTINGS_HPP__
#define __PROJECT_SETTINGS_HPP__

#include <filesystem>
#include <string>

struct ProjectSettings
{
  std::string m_name;
  std::string m_pdk_folder;
  std::string m_def_file;
  std::string m_guide_file;

  void
  save_to(const std::filesystem::path& path) const;

  void
  read_from(const std::filesystem::path& path);
};

#endif
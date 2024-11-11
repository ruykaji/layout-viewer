#include "Include/Ini.hpp"
#include "Include/Process.hpp"

int
main(int argc, char const* argv[])
{
  const ini::Config             config      = ini::parse("./config.ini");

  const std::string             pdk_folder  = config.at("PDK").get_as<std::string>("PATH");
  const std::string             design_path = config.at("DESIGN").get_as<std::string>("PATH");
  const std::string             guide_path  = config.at("DESIGN").get_as<std::string>("GUIDE");

  const lef::LEF                lef;
  const lef::Data               lef_data = lef.parse(pdk_folder);

  const def::DEF                def;
  def::Data                     def_data   = def.parse(design_path);

  const std::vector<guide::Net> guide_nets = guide::read(guide_path);

  process::fill_gcells(def_data, lef_data, guide_nets);

  return 0;
}
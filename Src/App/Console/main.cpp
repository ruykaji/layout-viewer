#include "Include/Ini.hpp"
#include "Include/Process.hpp"

int
main(int argc, char const* argv[])
{
  const ini::Config config      = ini::parse("./config.ini");

  const std::string pdk_folder  = config.at("PDK").get_as<std::string>("PATH");
  const std::string design_path = config.at("DESIGN").get_as<std::string>("PATH");

  const lef::LEF    lef;
  const lef::Data   lef_data = lef.parse(pdk_folder);

  const def::DEF    def;
  def::Data   def_data = def.parse(design_path);

  process::fill_gcells(def_data, lef_data);

  return 0;
}
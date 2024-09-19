#include "Include/Ini.hpp"
#include "Include/PDK.hpp"

int
main(int argc, char const* argv[])
{
  const ini::Config config     = ini::parse("./config.ini");
  
  const std::string pdk_folder = config.at("PDK").get_as<std::string>("PATH");

  const pdk::Reader pdk_reader;
  pdk_reader.compress(pdk_folder);

  return 0;
}

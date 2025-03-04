#include "Include/Ini.hpp"
#include "Include/Process.hpp"

int
main(int argc, char const* argv[])
{
  std::filesystem::path config_path = "./config.ini";

  if(argc > 1)
    {
      config_path = argv[1];
    }

  const ini::Config config = ini::parse(config_path);

  process::Process  proc;
  proc.set_path_pdk(config.at("PDK").get_as<std::string>("PATH"));
  proc.set_path_design(config.at("DESIGN").get_as<std::string>("PATH"));
  proc.set_path_guide(config.at("DESIGN").get_as<std::string>("GUIDE"));

  proc.prepare_data();
  std::cout << "1. Data hash been prepared" << std::endl;

  proc.collect_overlaps();
  std::cout << "2. Overlaps hash been collected" << std::endl;

  proc.apply_guide();
  std::cout << "3. Guide has been applied" << std::endl;

  proc.remove_empty_gcells();
  std::cout << "4. GCells has been processed" << std::endl;

  proc.make_dataset();

  return 0;
}
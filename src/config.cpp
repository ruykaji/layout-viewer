#include <filesystem>
#include <fstream>
#include <sstream>

#include "config.hpp"

Config::Config(int t_argc, char const* t_argv[])
{
    if (!m_configPath.empty() || t_argc == 3) {
        if (std::string(t_argv[1]) == "--config") {
            m_configPath = t_argv[2];
        } else {
            throw std::runtime_error("Invalid argument!");
        }

        if (std::filesystem::exists(m_configPath)) {
            std::ifstream inFile(m_configPath);
            std::string line {};

            while (std::getline(inFile, line)) {
                std::stringstream ss { line };
                std::string parameter {};

                if (std::getline(ss, parameter, '=')) {
                    if (m_settingsEnum.count(parameter) != 0) {
                        std::string settings {};

                        if (std::getline(ss, settings)) {
                            switch (m_settingsEnum[parameter]) {
                            case 0: {
                                m_mode = static_cast<Mode>(std::stoi(settings));
                                break;
                            }
                            case 1: {
                                m_pdkScaleFactor = std::stod(settings);
                                break;
                            }
                            case 2: {
                                m_cellSize = std::stoi(settings);
                                break;
                            }
                            case 3: {
                                m_borderSize = std::stoi(settings);
                                break;
                            }
                            default:
                                break;
                            }
                        }
                    } else {
                        throw std::runtime_error("There is no such parameter in config as '" + parameter + "'!");
                    }
                }
            }
        } else {
            throw std::runtime_error("Can't find config by path: " + m_configPath);
        }
    }
};
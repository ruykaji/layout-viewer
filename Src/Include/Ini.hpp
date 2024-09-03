#ifndef __INI_HPP__
#define __INI_HPP__

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace ini {

namespace details {

    /**
     * @brief Trim leading spaces
     *
     * @param line Line to trim
     */
    inline void ltrim(std::string& line)
    {
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    }

    /**
     * @brief Trim trailing  spaces
     *
     * @param line Line to trim
     */
    inline void rtrim(std::string& line)
    {
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
    }

} // namespace details

/**
 * @brief Section with setting in ini file.
 *
 */
class Section {
    friend std::unordered_map<std::string, Section> Parse(const std::filesystem::path& file_path);

public:
    /**
     * @brief Get the section setting as object of some exact type.
     *
     * @tparam Tp Setting type.
     * @param key Setting key.
     * @return Tp
     */
    template <typename Tp>
    Tp get_as(const std::string key)
    {
        static_assert(std::is_same_v<Tp, bool> || std::is_arithmetic_v<Tp> || std::is_same_v<Tp, std::string>, "No conversions exists for this type");

        if (!m_config.count(key)) {
            throw std::invalid_argument("Key \"" + key + "\" doesn't exists");
        }

        const std::string& value = m_config.at(key);

        if constexpr (std::is_same_v<Tp, bool>) {
            if (value == "true" || value == "1" || value == "on") {
                return true;
            }

            if (value == "false" || value == "0" || value == "off") {
                return false;
            }

            throw std::invalid_argument("Invalid value for key \"" + key + "\" and boolean type");
        }

        if constexpr (std::is_integral_v<Tp>) {
            try {
                Tp number = static_cast<Tp>(std::stoi(value.data()));
                return number;
            } catch (...) {
                throw std::invalid_argument("Invalid value for key \"" + key + "\" and arithmetic type");
            }
        }

        if constexpr (std::is_floating_point_v<Tp>) {
            try {
                Tp number = static_cast<Tp>(std::stod(value.data()));
                return number;
            } catch (...) {
                throw std::invalid_argument("Invalid value for key \"" + key + "\" and arithmetic type");
            }
        }

        if constexpr (std::is_same_v<Tp, std::string>) {
            return value;
        }
    }

private:
    std::unordered_map<std::string, std::string> m_config;
};

using Config = std::unordered_map<std::string, Section>;

Config Parse(const std::filesystem::path& file_path)
{
    if (!std::filesystem::exists(file_path)) {
        throw std::runtime_error("Failed to locate config file. Path \"" + file_path.string() + "\" does not exists");
    }

    Config config;
    std::string section_name = "";

    std::ifstream in_file(file_path);

    if (!in_file.is_open() || !in_file.good()) {
        throw std::runtime_error("Unexpected error occurred while trying to open file:\n" + file_path.string());
    }

    std::string line;

    while (std::getline(in_file, line)) {
        if (line.empty()) {
            continue;
        }

        details::ltrim(line);
        details::rtrim(line);

        if (line.at(0) == ';') {
            continue;
        }

        if (line.at(0) == '[') {
            section_name = line.substr(1, line.length() - 2);
            continue;
        }

        const std::size_t separator = line.find_first_of('=', 0);

        if (separator == std::string::npos) {
            throw std::invalid_argument("Expected key value pair but found nothing:\n\"" + line + "\"");
        }

        if (separator == 0) {
            throw std::invalid_argument("Expected key but found nothing:\n\"" + line + "\"");
        }

        if (separator == line.length() - 1) {
            throw std::invalid_argument("Expected value but found nothing:\n\"" + line + "\"");
        }

        if (!std::isspace(line.at(separator - 1)) || !std::isspace(line.at(separator + 1))) {
            throw std::invalid_argument("Unsupported format:\n\"" + line + "\"");
        }

        const std::string key = line.substr(0, separator - 1);
        const std::string value = line.substr(separator + 2);

        config[section_name].m_config[key] = value;
    }

    return config;
}

} // namespace ini

#endif
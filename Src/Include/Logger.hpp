#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Include/Macro.hpp"

namespace logger {

enum class Options : uint8_t {
    NONE = 0,
    CONSOLE_PRINT = 1 << 0
};

inline constexpr Options operator|(const Options& lhs, const Options& rhs)
{
    return static_cast<Options>(static_cast<std::underlying_type_t<Options>>(lhs) | static_cast<std::underlying_type_t<Options>>(rhs));
}

inline constexpr Options operator&(const Options& lhs, const Options& rhs)
{
    return static_cast<Options>(static_cast<std::underlying_type_t<Options>>(lhs) & static_cast<std::underlying_type_t<Options>>(rhs));
}

enum class Level {
    INFO = 0,
    DEBUG,
    SUCCESS,
    WARNING,
    ERROR
};

class Logger {
public:
    /**
     * @brief Construct a new Logger object.
     *
     * @param file_name Logger file name.
     * @param options Logger options.
     */
    Logger(std::filesystem::path file_name, Options options = Options::NONE)
        : m_options(options)
    {
        const std::filesystem::path log_folder_path = std::filesystem::current_path() / "logs";

        if (!std::filesystem::exists(log_folder_path)) {
            std::filesystem::create_directory(log_folder_path);
        }

        const std::filesystem::path log_file_path = log_folder_path / file_name;

        m_file_stream.open(log_file_path, std::ios::ate);
    }

    ~Logger()
    {
        if (m_file_stream.is_open() && m_file_stream.good()) {
            m_file_stream.close();
        }
    }

    NON_COPYABLE(Logger)
    NON_MOVABLE(Logger)

    /**
     * @brief Write log message.
     *
     * @param message Message.
     * @param level Log level.
     */
    void log(const std::string_view message, Level level)
    {
        if (!m_file_stream.is_open() || !m_file_stream.good()) {
            throw std::runtime_error("Unable to write to file. File is inaccessible.");
        }

        const std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        const std::tm* tm = std::localtime(&time);

        std::stringstream ss;
        ss << std::put_time(tm, "%d-%m-%Y %H:%M:%S");

        std::string result = "[" + ss.str() + "]";

        switch (level) {
        case Level::INFO: {
            result += " [INFO]    ";
            break;
        }
        case Level::DEBUG: {
            result += " [DEBUG]   ";
            break;
        }
        case Level::SUCCESS: {
            result += " [SUCCESS] ";
            break;
        }
        case Level::WARNING: {
            result += " [WARNING] ";
            break;
        }
        case Level::ERROR: {
            result += " [ERROR]   ";
            break;
        }
        default: {
            throw std::runtime_error("Unexpected log level.");
            break;
        }
        }

        result += message;

        if ((m_options & Options::CONSOLE_PRINT) == Options::CONSOLE_PRINT) {
            std::cout << result << '\n'
                      << std::flush;
        }

        m_file_stream << result << '\n'
                      << std::flush;
    }

private:
    std::ofstream m_file_stream;
    Options m_options;
};

} // namespace logger

#endif
#include <iostream>
#include <sstream>

#include "Include/Logger.hpp"

namespace logger
{

/** =============================== CONSTRUCTORS ================================= */

Logger::Logger(std::filesystem::path file_name, Options options) : m_options(options)
{
  const std::filesystem::path log_folder_path = std::filesystem::current_path() / "logs";

  if(!std::filesystem::exists(log_folder_path))
    {
      std::filesystem::create_directory(log_folder_path);
    }

  const std::filesystem::path log_file_path = log_folder_path / file_name;

  m_file_stream.open(log_file_path, std::ios::ate);
}

Logger::~Logger()
{
  if(m_file_stream.is_open() && m_file_stream.good())
    {
      m_file_stream.close();
    }
}

/** =============================== PUBLIC METHODS =============================== */

void
Logger::log(const std::string_view message, Level level)
{
  if(!m_file_stream.is_open() || !m_file_stream.good())
    {
      throw std::runtime_error("Unable to write to file. File is inaccessible.");
    }

  const std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  const std::tm*    tm   = std::localtime(&time);

  std::stringstream ss;
  ss << std::put_time(tm, "%d-%m-%Y %H:%M:%S");

  std::string result = "[" + ss.str() + "]";

  switch(level)
    {
    case Level::INFO:
      {
        result += " [INFO]    ";
        break;
      }
    case Level::DEBUG:
      {
        result += " [DEBUG]   ";
        break;
      }
    case Level::SUCCESS:
      {
        result += " [SUCCESS] ";
        break;
      }
    case Level::WARNING:
      {
        result += " [WARNING] ";
        break;
      }
    case Level::ERROR:
      {
        result += " [ERROR]   ";
        break;
      }
    default:
      {
        throw std::runtime_error("Unexpected log level.");
        break;
      }
    }

  result += message;

  if((m_options & Options::CONSOLE_PRINT) == Options::CONSOLE_PRINT)
    {
      std::cout << result << '\n' << std::flush;
    }

  m_file_stream << result << '\n' << std::flush;
}

} // namespace logger

#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <chrono>
#include <filesystem>
#include <fstream>

#include "Include/Macro.hpp"

namespace logger
{

enum class Options : uint8_t
{
  NONE          = 0,
  CONSOLE_PRINT = 1 << 0
};

inline constexpr Options
operator|(const Options& lhs, const Options& rhs)
{
  return static_cast<Options>(static_cast<std::underlying_type_t<Options>>(lhs) | static_cast<std::underlying_type_t<Options>>(rhs));
}

inline constexpr Options
operator&(const Options& lhs, const Options& rhs)
{
  return static_cast<Options>(static_cast<std::underlying_type_t<Options>>(lhs) & static_cast<std::underlying_type_t<Options>>(rhs));
}

enum class Level
{
  INFO = 0,
  DEBUG,
  SUCCESS,
  WARNING,
  ERROR
};

class Logger
{
public:
  NON_COPYABLE(Logger)
  NON_MOVABLE(Logger)

  /** =============================== CONSTRUCTORS ================================= */

  /**
   * @brief Constructs a new Logger object.
   *
   * @param file_name File name.
   * @param options Options.
   */
  Logger(std::filesystem::path file_name, Options options = Options::NONE);

  ~Logger();

public:
  /** =============================== PUBLIC METHODS =============================== */

  /**
   * @brief Writes log message.
   *
   * @param message Message.
   * @param level Log level.
   */
  void
  log(const std::string_view message, Level level);

private:
  std::ofstream m_file_stream; ///> File stream.
  Options       m_options;     ///> Holds the option of the logger.
};

} // namespace logger

#endif
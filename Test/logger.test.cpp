#include <gtest/gtest.h>

#include "Include/Logger.hpp"

TEST(LoggerTest, Open_Write_Close)
{
  logger::Logger file_logger("test-log.log", logger::Options::CONSOLE_PRINT);

  file_logger.log("test info message", logger::Level::INFO);
  file_logger.log("test debug message", logger::Level::DEBUG);
  file_logger.log("test success message", logger::Level::SUCCESS);
  file_logger.log("test warning message", logger::Level::WARNING);
  file_logger.log("test error message", logger::Level::ERROR);
}

int
main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

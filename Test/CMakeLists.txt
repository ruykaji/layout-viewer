find_package(GTest CONFIG REQUIRED)
   if(NOT GTest_FOUND)
   message(FATAL_ERROR "Google Test not found!")
endif()

enable_testing()

include(GoogleTest)

add_executable(LoggerTest logger.test.cpp)
target_link_libraries(LoggerTest GTest::gtest_main pthread)
gtest_discover_tests(LoggerTest)
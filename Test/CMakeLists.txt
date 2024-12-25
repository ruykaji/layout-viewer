find_package(GTest CONFIG REQUIRED)
   if(NOT GTest_FOUND)
   message(FATAL_ERROR "Google Test not found!")
endif()

enable_testing()

include(GoogleTest)

add_executable(LoggerTest logger.test.cpp)
target_link_libraries(LoggerTest Logger GTest::gtest_main pthread)
gtest_discover_tests(LoggerTest)

add_executable(IniTest ini.test.cpp)
target_link_libraries(IniTest Ini GTest::gtest_main pthread)
gtest_discover_tests(IniTest)

add_executable(MatrixTest matrix.test.cpp)
target_link_libraries(MatrixTest Matrix GTest::gtest_main pthread)
gtest_discover_tests(MatrixTest)

add_executable(RGBTest rgb.test.cpp)
target_link_libraries(RGBTest RGB GTest::gtest_main pthread)
gtest_discover_tests(RGBTest)

add_executable(GeometryTest geometry.test.cpp)
target_link_libraries(GeometryTest Geometry GTest::gtest_main pthread)
gtest_discover_tests(GeometryTest)
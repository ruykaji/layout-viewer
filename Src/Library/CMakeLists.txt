# Ini libary
add_library(Ini Ini.cpp)

# Logger library
add_library(Logger Logger.cpp)

# Matrix library
add_library(Matrix Matrix.cpp)

# RGB library
add_library(RGB RGB.cpp)
target_link_libraries(RGB libz.so)

# Utils library
add_library(Utils Utils.cpp)

# LEF library
add_library(LEF LEF.cpp)
target_include_directories(LEF PRIVATE ${CMAKE_SOURCE_DIR}/External/lef/include)
target_link_libraries(LEF PRIVATE  ${CMAKE_SOURCE_DIR}/External/lef/lib/liblef.a)
# Ini libary
add_library(Ini Ini.cpp)

# Logger library
add_library(Logger Logger.cpp)

# Matrix library
add_library(Matrix Matrix.cpp)

# RGB library
add_library(RGB RGB.cpp)
target_link_libraries(RGB libz.so)
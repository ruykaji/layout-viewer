set(TARGET_NAME reader)

file(GLOB_RECURSE SRC *.cpp)
file(GLOB_RECURSE HDR *.hpp)

add_library(${TARGET_NAME} ${SRC} ${HDR})

target_include_directories(${TARGET_NAME} PUBLIC include)
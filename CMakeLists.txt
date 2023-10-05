cmake_minimum_required(VERSION 3.19)
project(main LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/external/def/include
    ${CMAKE_SOURCE_DIR}/external/lef/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/decoder
    ${CMAKE_SOURCE_DIR}/src/encoder
    ${CMAKE_SOURCE_DIR}/src/neural_network
    ${CMAKE_SOURCE_DIR}/src/viewer
)

find_library(DEF NAMES libdef.a PATHS ${CMAKE_SOURCE_DIR}/external/def/lib)
find_library(LEF NAMES liblef.a PATHS ${CMAKE_SOURCE_DIR}/external/lef/lib)

file(GLOB_RECURSE SRC ${CMAKE_SOURCE_DIR}/src/**/*.cpp)
file(GLOB_RECURSE HDR ${CMAKE_SOURCE_DIR}/src/**/*.hpp)

add_executable(main main.cpp ${SRC} ${HDR})

target_link_libraries(main PRIVATE ${DEF} ${LEF} Qt6::Core Qt6::Gui Qt6::Widgets)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
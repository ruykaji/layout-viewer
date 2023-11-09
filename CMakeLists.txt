cmake_minimum_required(VERSION 3.19)
project(main LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CUDA_ARCHITECTURES 86)

find_library(DEF NAMES libdef.a PATHS ${CMAKE_SOURCE_DIR}/external/def/lib)
find_library(LEF NAMES liblef.a PATHS ${CMAKE_SOURCE_DIR}/external/lef/lib)

find_package(OpenCV REQUIRED)
find_package(Torch REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS})

include_directories(
    ${CMAKE_SOURCE_DIR}/external/def/include
    ${CMAKE_SOURCE_DIR}/external/lef/include
    ${CMAKE_SOURCE_DIR}/src
)

file(GLOB GLOBAL ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.hpp)
file(GLOB ENCODER ${CMAKE_SOURCE_DIR}/src/encoder/*.cpp ${CMAKE_SOURCE_DIR}/src/encoder/*.hpp)
file(GLOB PDK ${CMAKE_SOURCE_DIR}/src/pdk/*.cpp ${CMAKE_SOURCE_DIR}/src/pdk/*.hpp)
file(GLOB_RECURSE NEURAL_NETWORK ${CMAKE_SOURCE_DIR}/src/neural_network/*.cpp ${CMAKE_SOURCE_DIR}/src/neural_network/*.hpp)
file(GLOB VIEWER ${CMAKE_SOURCE_DIR}/src/viewer/*.cpp ${CMAKE_SOURCE_DIR}/src/viewer/*.hpp)

# Desktop Version
set(DESKTOP ${GLOBAL} ${ENCODER} ${PDK} ${VIEWER})
add_executable(desktop desktop.cpp ${DESKTOP})
target_link_libraries(desktop PRIVATE ${DEF} ${LEF} ${TORCH_LIBRARIES} Qt6::Core Qt6::Gui Qt6::Widgets)

# Console Version
set(CONSOLE ${GLOBAL} ${ENCODER} ${PDK} ${NEURAL_NETWORK})
add_executable(console console.cpp ${CONSOLE})
target_link_libraries(console PRIVATE ${DEF} ${LEF} ${TORCH_LIBRARIES} ${OpenCV_LIBS})
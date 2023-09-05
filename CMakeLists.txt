cmake_minimum_required(VERSION 3.19)
project(main LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SRC main.cpp)
# set(HDR ../../../Include/PaintData.hpp ../../../Include/LayerSelectWidget.hpp ../../../Include/GeometryWidget.hpp ../../../Include/LayoutWidget.hpp)

add_executable(main ${SRC})

target_link_libraries(main PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets reader)
target_include_directories(main PUBLIC ${SHARED_INCLUDE_DIRS})

set (CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "D:\\qt\\5.12.12\\mingw73_64\\")

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

add_subdirectory(lib)
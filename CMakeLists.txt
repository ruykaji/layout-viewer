cmake_minimum_required(VERSION 3.19)
project(main LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include_directories(/usr/local/include)

set(DEF /usr/local/lib/def/libdef.a)
set(LEF /usr/local/lib/lef/liblef.a)

set(SRC main.cpp)
set(HDR)

link_libraries(/usr/local/lib/lef /usr/local/lib/def)

add_executable(main ${SRC} ${HDR})

target_link_libraries(main PRIVATE ${DEF} ${LEF} Qt6::Core Qt6::Gui Qt6::Widgets)
target_include_directories(main PUBLIC include)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
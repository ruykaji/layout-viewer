add_library(Ini Ini.cpp)
add_library(Logger Logger.cpp)
add_library(Matrix Matrix.cpp)
add_library(GlobalUtils GlobalUtils.cpp)
add_library(Pin Pin.cpp)
add_library(Graph Graph.cpp)
add_library(Guide Guide.cpp)

add_library(Geometry Geometry.cpp)
target_link_libraries(Geometry Clipper2)

add_library(LEF LEF.cpp)
target_include_directories(LEF PUBLIC ${CMAKE_SOURCE_DIR}/External/lef/include)
target_link_libraries(LEF PUBLIC ${CMAKE_SOURCE_DIR}/External/lef/lib/liblef.a GlobalUtils Pin Geometry)

add_subdirectory(DEF)
add_library(Algorithms Algorithms.cpp)
target_link_libraries(Algorithms PUBLIC Graph Matrix)

add_library(Process Process.cpp)
target_link_libraries(Process PUBLIC LEF DEF Guide Matrix Algorithms)

add_subdirectory(GUI)
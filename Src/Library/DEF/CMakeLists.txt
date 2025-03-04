add_library(DEF DEF.cpp GCell.cpp Utils.cpp)
target_include_directories(DEF PUBLIC ${CMAKE_SOURCE_DIR}/External/def/include)
target_link_libraries(DEF PUBLIC ${CMAKE_SOURCE_DIR}/External/def/lib/libdef.a GlobalUtils Pin Geometry)
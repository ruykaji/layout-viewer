# Ini libary
add_library(Ini Ini.cpp)

# Logger library
add_library(Logger Logger.cpp)

# Matrix library
add_library(Matrix Matrix.cpp)

# RGB library
add_library(RGB RGB.cpp)
target_link_libraries(RGB PUBLIC libz.so)

# Utils library
add_library(Utils Utils.cpp)

# Pin library
add_library(Pin Pin.cpp)

# LEF library
add_library(LEF LEF.cpp)
target_include_directories(LEF PUBLIC ${CMAKE_SOURCE_DIR}/External/lef/include)
target_link_libraries(LEF PUBLIC ${CMAKE_SOURCE_DIR}/External/lef/lib/liblef.a Utils Pin)

# DEF library
add_library(DEF DEF.cpp)
target_include_directories(DEF PUBLIC ${CMAKE_SOURCE_DIR}/External/def/include)
target_link_libraries(DEF PUBLIC ${CMAKE_SOURCE_DIR}/External/def/lib/libdef.a Utils Pin)

# Guide library
add_library(Guide Guide.cpp)

# Process library
add_library(Process Process.cpp)
target_link_libraries(Process PUBLIC LEF DEF Guide Matrix)

# Qt widgets
set(QT_SOURCES 
   MainWindow.cpp 
   ViewerWidget.cpp 
   ProjectSettingsWidget.cpp
   StagesWidget.cpp)
set(QT_HEADERS 
   ${CMAKE_SOURCE_DIR}/Src/Include/MainWindow.hpp 
   ${CMAKE_SOURCE_DIR}/Src/Include/ViewerWidget.hpp 
   ${CMAKE_SOURCE_DIR}/Src/Include/ProjectSettingsWidget.hpp
   ${CMAKE_SOURCE_DIR}/Src/Include/StagesWidget.hpp)

add_library(MainWindow ${QT_SOURCES} ${QT_HEADERS})
target_link_libraries(MainWindow PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Process)
set(QT_SOURCES 
   MainWindow.cpp 
   MainScene.cpp
   ExamineScene.cpp
   ProjectSettings.cpp
   CreateProject.cpp
   Information.cpp)
set(QT_HEADERS 
   ${CMAKE_SOURCE_DIR}/Src/Include/GUI/MainWindow.hpp 
   ${CMAKE_SOURCE_DIR}/Src/Include/GUI/MainScene.hpp 
   ${CMAKE_SOURCE_DIR}/Src/Include/GUI/ExamineScene.hpp 
   ${CMAKE_SOURCE_DIR}/Src/Include/GUI/ProjectSettings.hpp
   ${CMAKE_SOURCE_DIR}/Src/Include/GUI/CreateProject.hpp
   ${CMAKE_SOURCE_DIR}/Src/Include/GUI/Information.hpp)

add_library(MainWindow ${QT_SOURCES} ${QT_HEADERS})
target_link_libraries(MainWindow PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Process GLU)
qt_add_resources(RESOURCES ${CMAKE_SOURCE_DIR}/Src/Assets/assets.qrc)

add_executable(${PROJECT_NAME}_gui main.cpp ${RESOURCES})
target_link_libraries(${PROJECT_NAME}_gui PRIVATE MainWindow)
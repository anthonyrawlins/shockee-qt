cmake_minimum_required(VERSION 3.16)
project(Shockee VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets SerialPort PrintSupport)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/serialcommunicator.cpp
    src/datalogger.cpp
    src/plotwidget.cpp
    src/calibrationdialog.cpp
)

set(HEADERS
    src/mainwindow.h
    src/serialcommunicator.h
    src/datalogger.h
    src/plotwidget.h
    src/calibrationdialog.h
)

set(UI_FILES
    ui/mainwindow.ui
    ui/calibrationdialog.ui
)

qt_add_executable(Shockee ${SOURCES} ${HEADERS})
qt_add_resources(Shockee "resources" FILES icons/shockee.png)

target_link_libraries(Shockee
    Qt6::Core
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::PrintSupport
)

# Create directories
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ui)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/arduino)
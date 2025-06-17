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

# Configure as macOS app bundle
set_target_properties(Shockee PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.shockee.dyno"
    MACOSX_BUNDLE_BUNDLE_NAME "Shockee"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION} - Suspension Dyno"
    MACOSX_BUNDLE_INFO_STRING "Shockee Motorbike Suspension Dyno Software"
    MACOSX_BUNDLE_COPYRIGHT "© 2024 Anthony Rawlins"
)

target_link_libraries(Shockee PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::PrintSupport
)

# Installation targets for Ubuntu packaging
include(GNUInstallDirs)

install(TARGETS Shockee
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

# Install documentation
install(FILES
    README.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

# Install Arduino sketch
install(FILES
    arduino/shockee_sensors.ino
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/arduino
)

# Install sample sessions
install(DIRECTORY sessions/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/shockee/sessions
    FILES_MATCHING PATTERN "*.json"
)

# Install desktop file and icon on Linux
if(UNIX AND NOT APPLE)
    install(FILES
        debian/shockee.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    
    # Install icon if it exists
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/icons/icon.png)
        install(FILES
            icons/icon.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
            RENAME shockee.png
        )
    endif()
endif()

# Create directories
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ui)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/arduino)
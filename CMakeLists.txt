cmake_minimum_required(VERSION 3.16)

project(revolutionary-audio-player VERSION 0.1 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ./target/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./target/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ./target/lib)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/mainwindow.ui
        src/musicitem.h
)

qt_add_executable(revolutionary-audio-player
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(revolutionary-audio-player PRIVATE Qt6::Widgets Qt6::Multimedia)

set_target_properties(revolutionary-audio-player PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS revolutionary-audio-player
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(revolutionary-audio-player)
endif()
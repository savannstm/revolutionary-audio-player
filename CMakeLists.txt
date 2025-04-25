cmake_minimum_required(VERSION 3.16)

project(rap VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY target/bin)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Concurrent)
find_package(JUCE REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.hpp
    src/*.cpp
    src/*.h
    src/*.c
    src/*.ui
    src/*.ts
)

qt_add_executable(rap
    MANUAL_FINALIZATION
    ${SOURCES}
)

set_target_properties(rap PROPERTIES WIN32_EXECUTABLE TRUE)

file(GLOB SUBDIRS LIST_DIRECTORIES true src/*)
target_include_directories(rap PRIVATE ${SUBDIRS})

target_include_directories(rap PRIVATE $ENV{FFMPEG_INCLUDE_DIRS})
target_link_directories(rap PRIVATE $ENV{FFMPEG_LIBRARY_DIRS})

find_path(RAPIDHASH_INCLUDE_DIRS "rapidhash.h")
target_include_directories(rap PRIVATE ${RAPIDHASH_INCLUDE_DIRS})

configure_file(
    src/version/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(rap PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Concurrent
    juce_dsp
    avcodec
    avformat
    avutil
    swresample
)


include(GNUInstallDirs)
install(TARGETS rap
    BUNDLE DESTINATION .
)

qt_finalize_executable(rap)
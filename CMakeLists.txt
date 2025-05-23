# Minimum CMake version required
cmake_minimum_required(VERSION 3.16)

# Project definition
set(PROJECT_NAME rap)
set(PROJECT_VERSION "0.4.1")
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

# Define option for static builds
option(BUILD_STATIC "Link with static libraries" OFF)

# Set standards for C and C++
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directory for the compiled binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY target/bin)

# Enable Qt6 automatic processing (UIC, MOC, RCC)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Configure MSVC runtime linkage (static runtime, Debug/Release aware)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Concurrent LinguistTools Gui)

find_path(FFMPEG_INCLUDE_DIR "avcodec.h" PATH_SUFFIXES ffmpeg)

find_library(AVCODEC_LIBRARY avcodec)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWRESAMPLE_LIBRARY swresample)
find_library(AVFILTER_LIBRARY avfilter)

if(NOT (AVCODEC_LIBRARY AND AVFORMAT_LIBRARY AND AVUTIL_LIBRARY AND SWRESAMPLE_LIBRARY AND AVFILTER_LIBRARY))
    message(FATAL_ERROR "FFmpeg libraries not found, specify path to them using `-DFFMPEG_INCLUDE_DIR=...`")
endif()

find_path(CIMG_INCLUDE_DIR "CImg.h")
if(NOT CIMG_INCLUDE_DIR)
    message(FATAL_ERROR "CImg.h not found, specify path to it using `-DCIMG_INCLUDE_DIR=...`")
endif()

find_path(RAPIDHASH_INCLUDE_DIRS "rapidhash.h")
if(NOT RAPIDHASH_INCLUDE_DIRS)
    message(FATAL_ERROR "rapidhash.h not found, specify path to it using `-DRAPIDHASH_INCLUDE_DIRS=...`")
endif()

# Configure the project version header
configure_file(
    src/version/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

# Collect all source files recursively
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.c
    src/*.hpp
    src/*.h
    src/*.ui
    src/*.ts
)

# Collect translation source files
file(GLOB TRANSLATION_TS_FILES translations/*)

# Add translations
qt6_add_translations(
    ${PROJECT_NAME}
    TS_FILES ${TRANSLATION_TS_FILES}
    SOURCES ${SOURCES}
    QM_FILES_OUTPUT_VARIABLE TRANSLATION_QM_FILES
)

# Create the executable, but do not yet finalize it
set(WINDOWS_APPLICATION_ICON "resources/${PROJECT_NAME}-logo.rc")

qt_add_executable(${PROJECT_NAME}
    MANUAL_FINALIZATION
    ${SOURCES}
    ${QM_FILES}
    ${WINDOWS_APPLICATION_ICON}
)

# Set properties (e.g., no console window for Windows app)
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

# Include the directory for generated files (e.g., version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Include additional source subdirectories
file(GLOB SUBDIRS LIST_DIRECTORIES true src/*)
file(GLOB ICONS icons/*)

# Include paths
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SUBDIRS}
    ${RAPIDHASH_INCLUDE_DIRS}
    ${CIMG_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Concurrent
    avcodec
    avfilter
    avformat
    avutil
    swresample
)

# Libraries to link on Linux
if (UNIX AND NOT APPLE)
    if (BUILD_STATIC)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a .so)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE
        -lX11
        -lxcb
        -lXau
        -lXdmcp
        -lpthread
        -ldrm
        -lintl
        -latomic
        -lm
    )
endif()

# Install the built executable
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
)

# Finalize the Qt executable setup
qt_finalize_executable(${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/translations
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${TRANSLATION_QM_FILES}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/translations
    COMMENT "Copying translation (.qm) files to the build configuration output directory"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/icons
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ICONS}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/icons
    COMMENT "Copying icons to the build configuration output directory"
)
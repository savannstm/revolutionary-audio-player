# Minimum CMake version required
cmake_minimum_required(VERSION 3.25)

# Project definition
set(PROJECT_NAME rap)
set(PROJECT_VERSION "0.11.0")
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

option(ENABLE_PROJECTM "Enable projectM visualizer. If this option is enabled, projectM visualizer executable will be built alongside the application. Disable if you don't want audio visualization through OpenGL." ON)

option(MA_NO_WAV "Disable WAV support in miniaudio" ON)
option(MA_NO_MP3 "Disable MP3 support in miniaudio" ON)
option(MA_NO_FLAC "Disable FLAC support in miniaudio" ON)
option(MA_NO_DECODING "Disable all decoding in miniaudio" ON)
option(MA_NO_ENCODING "Disable all encoding in miniaudio" ON)

option(MA_NO_RESOURCE_MANAGER "Disable resource manager in miniaudio" ON)
option(MA_NO_NODE_GRAPH "Disable node graph in miniaudio" ON)
option(MA_NO_ENGINE "Disable high-level engine in miniaudio" ON)
option(MA_NO_GENERATION "Disable sound generation utilities in miniaudio" ON)

option(MA_ENABLE_ONLY_SPECIFIC_BACKENDS "Enable only selected backends in miniaudio" ON)
option(MA_ENABLE_PULSEAUDIO "Enable PulseAudio backend in miniaudio. Only used with enabled MA_ENABLE_ONLY_SPECIFIC_BACKENDS" ON)
option(MA_ENABLE_WASAPI "Enable WASAPI backend in miniaudio. Only used with enabled MA_ENABLE_ONLY_SPECIFIC_BACKENDS" ON)

option(MA_DEBUG_OUTPUT "Enable miniaudio debug output" ON)

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

if (LINUX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Gui Network LinguistTools)

find_path(FFMPEG_INCLUDE_DIR "avcodec.h" PATH_SUFFIXES ffmpeg)

find_library(AVFILTER_LIBRARY avfilter)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVCODEC_LIBRARY avcodec)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWRESAMPLE_LIBRARY swresample)

if(NOT (AVCODEC_LIBRARY AND AVFORMAT_LIBRARY AND AVUTIL_LIBRARY AND SWRESAMPLE_LIBRARY AND AVFILTER_LIBRARY))
    message(FATAL_ERROR "FFmpeg libraries not found, specify path to them using `-DFFMPEG_INCLUDE_DIR=...`")
endif()

find_path(RAPIDHASH_INCLUDE_DIRS "rapidhash.h")
if(NOT RAPIDHASH_INCLUDE_DIRS)
    message(FATAL_ERROR "rapidhash.h not found, specify path to it using `-DRAPIDHASH_INCLUDE_DIRS=...`")
endif()

find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h")
if(NOT MINIAUDIO_INCLUDE_DIRS)
    message(FATAL_ERROR "miniaudio.h not found, specify path to it using `-DMINIAUDIO_INCLUDE_DIRS=...`")
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

list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Visualizer/VisualizerSidecar.cpp)

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

# Build mode macros for usage in C++
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG_BUILD>
        $<$<CONFIG:Release>:RELEASE_BUILD>
)

# Apply enabled Miniaudio options as compile definitions
foreach(opt
    MA_NO_WAV
    MA_NO_MP3
    MA_NO_FLAC
    MA_NO_DECODING
    MA_NO_ENCODING
    MA_NO_RESOURCE_MANAGER
    MA_NO_NODE_GRAPH
    MA_NO_ENGINE
    MA_NO_GENERATION
    MA_ENABLE_ONLY_SPECIFIC_BACKENDS
    MA_ENABLE_PULSEAUDIO
    MA_ENABLE_WASAPI
    MA_DEBUG_OUTPUT
)
    if(${opt})
        target_compile_definitions(${PROJECT_NAME} PRIVATE ${opt})
    endif()
endforeach()

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
    ${MINIAUDIO_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    avfilter
    avformat
    avcodec
    avutil
    swresample
)

# Compile visualizer sidecar, if projectM is enabled
if (ENABLE_PROJECTM)
    add_executable(rap-visualizer
        src/Visualizer/VisualizerSidecar.cpp
    )

    find_package(projectM4 REQUIRED)
    find_package(glfw3 REQUIRED)

    set_target_properties(rap-visualizer PROPERTIES WIN32_EXECUTABLE TRUE)
    target_include_directories(rap-visualizer PRIVATE src/Visualizer)

    target_link_libraries(rap-visualizer PRIVATE
        libprojectM::projectM
        glfw
    )

    if (WIN32)
        find_package(GLEW REQUIRED)
        target_link_libraries(rap-visualizer PRIVATE
            GLEW::GLEW
        )
    endif()

    if (LINUX)
        find_package(OpenGL REQUIRED)
        target_link_libraries(rap-visualizer PRIVATE
            OpenGL::GL
        )
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PROJECTM
    )
endif()

# Libraries to link on Linux
if (LINUX)
    find_package(Qt6 REQUIRED COMPONENTS Svg WaylandClient)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Svg
        Qt6::WaylandClient
        -lpulse
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

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/visualizer
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/visualizer
    COMMENT "Copying visualizer assets directory"
)
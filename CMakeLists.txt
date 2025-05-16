# Minimum CMake version required
cmake_minimum_required(VERSION 3.16)

# Project definition
set(PROJECT_NAME rap)
set(PROJECT_VERSION "0.2.2")
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

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
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Concurrent LinguistTools Gui)
find_package(JUCE REQUIRED)
find_package(CImg CONFIG REQUIRED)

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

# Find external includes (rapidhash)
find_path(RAPIDHASH_INCLUDE_DIRS "rapidhash.h")

# Configure the project version header
configure_file(
    src/version/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

# Include paths
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SUBDIRS}
    $ENV{FFMPEG_INCLUDE_DIRS}
    ${RAPIDHASH_INCLUDE_DIRS}
)

# Link directories
target_link_directories(${PROJECT_NAME} PRIVATE
    $ENV{FFMPEG_LIBRARY_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Concurrent
    juce_core
    juce_dsp
    avcodec
    avformat
    avutil
    swresample
    CImg::CImg
)

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
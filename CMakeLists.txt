# Minimum CMake version required
cmake_minimum_required(VERSION 3.16)

# Project definition
project(rap VERSION 0.1.0 LANGUAGES C CXX)

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
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Concurrent LinguistTools)
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

# (Optional) Create a target to update translations
qt_add_lupdate(rap
    SOURCES ${SOURCES}
    TS_FILES ${TRANSLATION_TS_FILES}
)

# Compile translations (.ts -> .qm)
qt_add_lrelease(rap
    TS_FILES ${TRANSLATION_TS_FILES}
    QM_FILES_OUTPUT_VARIABLE TRANSLATION_QM_FILES
)

# Create the executable, but do not yet finalize it
qt_add_executable(rap
    MANUAL_FINALIZATION
    ${SOURCES}
    ${QM_FILES}
)

# Set properties (e.g., no console window for Windows app)
set_target_properties(rap PROPERTIES WIN32_EXECUTABLE TRUE)

# Include the directory for generated files (e.g., version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Include additional source subdirectories
file(GLOB SUBDIRS LIST_DIRECTORIES true src/*)

# Find external includes (rapidhash)
find_path(RAPIDHASH_INCLUDE_DIRS "rapidhash.h")

# Configure the project version header
configure_file(
    src/version/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

# Include paths
target_include_directories(rap PRIVATE
    ${SUBDIRS}
    $ENV{FFMPEG_INCLUDE_DIRS}
    ${RAPIDHASH_INCLUDE_DIRS}
)

# Link directories
target_link_directories(rap PRIVATE
    $ENV{FFMPEG_LIBRARY_DIRS}
)

# Link libraries
target_link_libraries(rap PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Concurrent
    juce_dsp
    avcodec
    avformat
    avutil
    swresample
    CImg::CImg
)

# Install the built executable
include(GNUInstallDirs)
install(TARGETS rap
    BUNDLE DESTINATION .
)

# Finalize the Qt executable setup
qt_finalize_executable(rap)

add_custom_command(TARGET rap POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:rap>/translations
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${TRANSLATION_QM_FILES}
        $<TARGET_FILE_DIR:rap>/translations
    COMMENT "Copying translation (.qm) files to the build configuration output directory"
)
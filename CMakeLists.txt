cmake_minimum_required(VERSION 3.16)

project(rap VERSION 0.1 LANGUAGES C CXX)

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
find_package(TagLib REQUIRED)
find_package(ZLIB)
find_package(juce REQUIRED)

file(GLOB PROJECT_SOURCES src/*.cpp src/*.hpp src/*.h)

qt_add_executable(rap
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_include_directories(rap PRIVATE src)

# minimalistic builds
target_include_directories(rap PRIVATE $ENV{FFMPEG_INCLUDE_DIRS})
target_link_directories(rap PRIVATE $ENV{FFMPEG_LIBRARY_DIRS})

# full featured
# target_include_directories(rap PRIVATE ${FFMPEG_INCLUDE_DIRS})
# target_link_directories(rap PRIVATE ${FFMPEG_LIBRARY_DIRS})
# target_link_libraries(rap PRIVATE ${FFMPEG_LIBRARIES})

target_link_libraries(rap PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Concurrent
    TagLib::TagLib
    juce::juce_dsp
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
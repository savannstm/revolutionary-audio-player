cmake_minimum_required(VERSION 3.16)

project(rap VERSION 0.1 LANGUAGES CXX)
include(FetchContent)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ./target/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./target/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ./target/lib)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
find_package(TagLib REQUIRED)
find_package(SndFile REQUIRED)
FetchContent_MakeAvailable(juce)

file(GLOB PROJECT_SOURCES src/*.cpp src/*.hpp src/*.h)

qt_add_executable(rap
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    ${AUTOGEN}
)

target_link_libraries(rap PRIVATE Qt6::Widgets Qt6::Multimedia TagLib::TagLib SndFile::sndfile juce::juce_dsp)
target_include_directories(rap PRIVATE src)

include(GNUInstallDirs)
install(TARGETS rap
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(rap)
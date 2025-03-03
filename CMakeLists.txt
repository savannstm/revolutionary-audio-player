cmake_minimum_required(VERSION 3.16)

project(rap VERSION 0.1 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ./target/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./target/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ./target/lib)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    iir
    GIT_REPOSITORY https://github.com/berndporr/iir1.git
    GIT_TAG master
)

FetchContent_MakeAvailable(iir)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
find_package(TagLib REQUIRED)
find_package(SndFile REQUIRED)

file(GLOB PROJECT_SOURCES src/*.cpp src/*.h)

qt_add_executable(revolutionary-audio-player
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    ${AUTOGEN}
)

target_link_libraries(revolutionary-audio-player PRIVATE Qt6::Widgets Qt6::Multimedia)
target_link_libraries(revolutionary-audio-player PRIVATE TagLib::TagLib)
target_link_libraries(revolutionary-audio-player PRIVATE iir::iir_static)
target_link_libraries(revolutionary-audio-player PRIVATE SndFile::sndfile)

target_include_directories(revolutionary-audio-player PRIVATE src)

include(GNUInstallDirs)
install(TARGETS revolutionary-audio-player
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(revolutionary-audio-player)
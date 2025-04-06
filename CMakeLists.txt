cmake_minimum_required(VERSION 3.16)

project(rap VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY target/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY target/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY target/lib)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
find_package(TagLib REQUIRED)
find_package(juce REQUIRED)
find_package(FFmpeg REQUIRED COMPONENTS avcodec avformat avutil swresample swscale)

file(GLOB PROJECT_SOURCES src/*.cpp src/*.hpp src/*.h)

qt_add_executable(rap
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    ${AUTOGEN}
)

target_include_directories(rap PRIVATE src)
target_link_libraries(rap PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    TagLib::TagLib
    juce::juce_dsp
    avcodec
    avformat
    avutil
    swresample
    swscale
)

include(GNUInstallDirs)
install(TARGETS rap
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(rap)

if(WIN32)
    add_custom_command(TARGET rap POST_BUILD
        COMMAND windeployqt.exe \"$<TARGET_FILE:rap>\"
        COMMENT "Deploying Qt DLLs..."
    )
endif()

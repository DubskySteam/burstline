cmake_minimum_required(VERSION 3.25)

project(
    burstline
    VERSION 0.1.0
    DESCRIPTION "High-throughput HTTP load tester with a terminal user interface"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BURSTLINE_BUILD_TESTING "Build Burstline tests" ON)
option(BURSTLINE_ENABLE_WARNINGS "Enable compiler warnings for Burstline targets" ON)

add_library(burstline_build_settings INTERFACE)
add_library(burstline::build_settings ALIAS burstline_build_settings)

target_compile_features(burstline_build_settings INTERFACE cxx_std_20)

if(BURSTLINE_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(
            burstline_build_settings
            INTERFACE
                /W4
                /permissive-
        )
    else()
        target_compile_options(
            burstline_build_settings
            INTERFACE
                -Wall
                -Wextra
                -Wpedantic
        )
    endif()
endif()

include(CTest)

if(BUILD_TESTING AND BURSTLINE_BUILD_TESTING)
    message(STATUS "Testing is enabled; test targets will be added with implementation milestones.")
endif()

message(STATUS "Configuring Burstline ${PROJECT_VERSION}")
message(STATUS "C++ standard: C++${CMAKE_CXX_STANDARD}")

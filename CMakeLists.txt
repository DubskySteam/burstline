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
option(BURSTLINE_BUILD_DOCUMENTATION "Add the Doxygen documentation target when available" ON)

add_library(burstline_build_settings INTERFACE)
add_library(burstline::build_settings ALIAS burstline_build_settings)

target_compile_features(burstline_build_settings INTERFACE cxx_std_20)

if(BURSTLINE_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(
            burstline_build_settings
            INTERFACE
                /W4
                /EHsc
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

add_library(
    burstline_core
    src/command_line.cpp
    src/workload.cpp
)
add_library(burstline::core ALIAS burstline_core)

target_include_directories(
    burstline_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(burstline_core PUBLIC burstline::build_settings)

add_executable(
    burstline
    src/application.cpp
    src/main.cpp
)

target_include_directories(
    burstline
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(burstline PRIVATE burstline::core)

include(CTest)

if(BURSTLINE_BUILD_DOCUMENTATION)
    find_package(Doxygen QUIET)
    if(Doxygen_FOUND)
        configure_file(
            docs/Doxyfile.in
            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            @ONLY
        )
        add_custom_target(
            docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating Doxygen API documentation"
            VERBATIM
        )
    else()
        message(STATUS "Doxygen not found; the docs target is unavailable.")
    endif()
endif()

if(BUILD_TESTING AND BURSTLINE_BUILD_TESTING)
    add_test(NAME burstline.help COMMAND burstline --help)
    set_tests_properties(burstline.help PROPERTIES PASS_REGULAR_EXPRESSION "Usage:")

    add_executable(burstline_core_tests tests/workload_test.cpp)
    target_link_libraries(burstline_core_tests PRIVATE burstline::core)
    add_test(NAME burstline.core COMMAND burstline_core_tests)
endif()

message(STATUS "Configuring Burstline ${PROJECT_VERSION}")
message(STATUS "C++ standard: C++${CMAKE_CXX_STANDARD}")

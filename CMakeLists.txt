cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(cxxalg VERSION 1.0 LANGUAGES CXX)

add_library(cxxalg INTERFACE)
target_include_directories(cxxalg INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/")

option(BUILD_TEST "Generate the test target.")

if(BUILD_TEST)
    add_subdirectory(test)
endif()

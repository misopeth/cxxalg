cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(cxxalg VERSION 1.0 LANGUAGES CXX)

FILE(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hxx")

add_library(cxxalg ${headers})

set_target_properties(cxxalg PROPERTIES CXX_STANDARD 20)

target_include_directories(cxxalg
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

packageProject(
    NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
    INCLUDE_DESTINATION include/${PROJECT_NAME}
    DEPENDENCIES ""
)

option(CXXALG_TEST "Generate the test target.")

if(CXXALG_TEST)
    add_subdirectory(test)
endif()

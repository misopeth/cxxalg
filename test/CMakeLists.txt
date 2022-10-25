cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

FILE(GLOB_RECURSE test_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test/*.hxx")
FILE(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cxx")

add_executable(cxxalg-test ${test_headers} ${test_sources})

set_target_properties(cxxalg-test PROPERTIES CXX_STANDARD 20)

target_include_directories(cxxalg
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

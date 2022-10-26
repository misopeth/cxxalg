cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

FILE(GLOB_RECURSE test_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx")
FILE(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx")

add_executable(cxxalg-test ${test_headers} ${test_sources})
target_link_libraries(cxxalg-test PUBLIC cxxalg)

set_target_properties(cxxalg-test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

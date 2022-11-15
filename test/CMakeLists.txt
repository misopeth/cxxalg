cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

FILE(GLOB_RECURSE test_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx")
FILE(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx")

Include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.1.1
)

FetchContent_MakeAvailable(Catch2)

add_executable(cxxalg-test ${test_headers} ${test_sources})
target_link_libraries(cxxalg-test PRIVATE cxxalg Catch2::Catch2WithMain)

set_target_properties(cxxalg-test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

cmake_minimum_required(VERSION 3.16)
project(Themis CXX)

set(CMAKE_CXX_STANDARD 23)
#set(SRC_COMMON src/)

find_package(OpenMP REQUIRED)


#--- test executable ---
include(FetchContent)
FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/doctest/doctest.git GIT_TAG v2.5.3)
FetchContent_MakeAvailable(doctest)

add_executable(tests tests/test_main.cpp)

target_link_libraries(tests PRIVATE doctest::doctest)


enable_testing()
add_test(NAME tests COMMAND tests)

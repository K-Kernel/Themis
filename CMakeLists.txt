cmake_minimum_required(VERSION 3.16)
project(Themis CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS REQUIRED OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenMP REQUIRED)

add_library(themis src/main.cpp)
target_include_directories(themis PUBLIC include/themis/io.hpp)
target_link_libraries(themis PUBLIC OpenMP::OpenMP_CXX)


#--- test executable ---
include(FetchContent)
FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/doctest/doctest.git GIT_TAG v2.5.3)
FetchContent_MakeAvailable(doctest)

add_executable(tests tests/test_main.cpp)

target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(tests PRIVATE doctest::doctest)
target_link_libraries(tests PRIVATE themis)


enable_testing()
add_test(NAME tests COMMAND tests)

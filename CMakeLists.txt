cmake_minimum_required(VERSION 3.16)
project(Themis CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
  add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG _GLIBCXX_ASSERTIONS)
endif()

add_library(themis src/main.cpp)

target_include_directories(themis PUBLIC include/)
target_compile_options(themis PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

find_package(OpenMP REQUIRED)
target_link_libraries(themis PUBLIC OpenMP::OpenMP_CXX)


#--- test executable ---
include(FetchContent)
FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/doctest/doctest.git GIT_TAG v2.5.3)
FetchContent_MakeAvailable(doctest)

add_executable(tests tests/main.cpp tests/test_csv.cpp tests/test_infer.cpp)

target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
target_link_libraries(tests PRIVATE doctest::doctest)
target_link_libraries(tests PRIVATE themis)

add_executable(themis-describe tools/describe.cpp)
target_compile_options(themis-describe PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
target_link_libraries(themis-describe PRIVATE themis)

enable_testing()
add_test(NAME tests COMMAND tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

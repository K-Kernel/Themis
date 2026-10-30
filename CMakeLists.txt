cmake_minimum_required(VERSION 3.25)
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

#--- server dependencies, used only through the themis_serve target ---
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_NON_BLOCKING_GETADDRINFO OFF CACHE BOOL "" FORCE)
set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(httplib GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
                     GIT_TAG v0.26.0 GIT_SHALLOW TRUE SYSTEM)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
                     DOWNLOAD_EXTRACT_TIMESTAMP TRUE SYSTEM)
FetchContent_MakeAvailable(httplib json)

add_library(themis_serve INTERFACE)
target_link_libraries(themis_serve INTERFACE themis httplib::httplib nlohmann_json::nlohmann_json)

add_executable(tests tests/main.cpp tests/test_csv.cpp tests/test_infer.cpp tests/test_graph.cpp tests/test_opt.cpp tests/test_metrics.cpp tests/test_serve.cpp)

target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
target_link_libraries(tests PRIVATE doctest::doctest)
target_link_libraries(tests PRIVATE themis themis_serve)

add_executable(themis-describe tools/describe.cpp)
target_compile_options(themis-describe PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
target_link_libraries(themis-describe PRIVATE themis)

add_executable(themis-flights examples/flights.cpp)
target_compile_options(themis-flights PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
target_link_libraries(themis-flights PRIVATE themis)

add_executable(themis-bench-graph tools/bench_graph.cpp)
target_compile_options(themis-bench-graph PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
target_link_libraries(themis-bench-graph PRIVATE themis)

add_executable(themis-meet examples/meet.cpp)
target_compile_options(themis-meet PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
target_link_libraries(themis-meet PRIVATE themis)

add_executable(themis-bench-opt tools/bench_opt.cpp)
target_compile_options(themis-bench-opt PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
target_link_libraries(themis-bench-opt PRIVATE themis)

add_executable(themis-meet-serve examples/meet_serve.cpp)
target_compile_options(themis-meet-serve PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
target_link_libraries(themis-meet-serve PRIVATE themis_serve)

enable_testing()
add_test(NAME tests COMMAND tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

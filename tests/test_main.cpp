#include <cassert>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <themis/buffer.hpp>

TEST_CASE("Basic addition") { assert(1 + 1 == 2); }
TEST_CASE("Load a small file") {
  Buffer data = read_data("data/test_data1.txt");
  assert(sizeof(*data.file_buffer) ==
         sizeof(std::filesystem::file_size("data/test_data1.txt")));
}

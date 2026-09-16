#include <cassert>
#include <span>
#include <string_view>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <filesystem>
#include <themis/buffer.hpp>
#include <themis/io.hpp>
#include <themis/table.hpp>

TEST_CASE("Basic addition") { CHECK(1 + 1 == 2); }

TEST_CASE("Load a small file") {
  Buffer data = read_data("data/test_data1.txt");
  REQUIRE(data.buffer_view.size() ==
          std::filesystem::file_size("data/test_data1.txt"));
}

TEST_CASE("Test table struct") {
  Buffer buffer = read_data("data/test_data1.txt");

  std::string_view data(buffer.buffer_view.data(), buffer.buffer_view.size());

  std::vector<std::string_view> header{"name", "age", "score"};
  std::vector<std::string_view> cells{};

  cells.push_back(data.substr(0, 5));
  cells.push_back(data.substr(6, 2));
  cells.push_back(data.substr(9, 4));

  Table table{buffer, header, cells, 3};
  CHECK(table.ncols() == 3);
  CHECK(table.nrows() == 1);
  CHECK(table.at(0, 0) == "Alice");
  CHECK(table.at(0, 1) == "25");
  CHECK(table.at(0, 2) == "91.5");
}

TEST_CASE("Testing slice function") {
  Buffer buffer = read_data("data/test_data1.txt");
  Table table = slice_csv(buffer);
  REQUIRE(table.ncols() == 3);
  REQUIRE(table.nrows() == 5);
  REQUIRE(table.at(4, 2) == "89.9");
}

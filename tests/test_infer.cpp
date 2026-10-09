#include "helpers.hpp"
#include "themis/infer.hpp"
#include <cstdint>
#include <doctest/doctest.h>
#include <stdexcept>
#include <themis/buffer.hpp>
#include <themis/io.hpp>
#include <themis/table.hpp>
#include <vector>

using namespace themis;

Table typed(const std::string &name) {
  Table t = load(name, true);
  infer_types(t);
  return t;
};

TEST_CASE("Table with integers") {
  Table t = typed("integer.csv");
  auto s = t.ints(0);

  CHECK(std::vector<int>(s.begin(), s.end()) == std::vector<int>{1, -2, 30});
}

TEST_CASE("Table with double") {
  Table t = typed("double.csv");
  auto s = t.doubles(0);

  CHECK(std::vector<double>(s.begin(), s.end()) ==
        std::vector<double>{1.5, -2.0});
}

TEST_CASE("Table with string") {
  Table t = typed("string.csv");
  auto s = t.strings(0);

  CHECK(s[0] == "12abc");
  CHECK_THROWS_AS(t.ints(0), std::logic_error);
}

TEST_CASE("Table with null") {
  Table t = typed("nulls.csv");
  auto s = t.ints(1);

  CHECK(std::vector<int>(s.begin(), s.end()) == std::vector<int>{1, 0, 0, 4});
  CHECK(t.columns[1].valid == std::vector<uint8_t>{1, 0, 0, 1});
}

TEST_CASE("Table with space and plus") {
  Table t = typed("spaces_plus.csv");
  auto s = t.ints(0);
  auto v = t.ints(1);

  CHECK(s[0] == 42);
  CHECK(v[0] == 5);
}

TEST_CASE("Table with a very big number") {
  Table t = typed("overflow.csv");
  auto s = t.doubles(0);

  CHECK(s[0] == 1e20);
}

TEST_CASE("Null table") {
  Table t = typed("all_null.csv");
  auto s = t.strings(1);

  CHECK(t.column(1).valid == std::vector<uint8_t>{0, 0});
}

TEST_CASE("Infering without proper loading") {
  Table t = load("integer.csv");

  CHECK_THROWS_AS(t.ints(0), std::logic_error);
}

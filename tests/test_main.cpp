#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cassert>
#include <doctest/doctest.h>
#include <string>
#include <string_view>
#include <themis/buffer.hpp>
#include <themis/io.hpp>
#include <themis/table.hpp>
#include <vector>

using Grid = std::vector<std::vector<std::string>>;

Table load(const std::string &name) {
  return slice_csv(read_csv("data/" + name));
}

void check_grid(const Table &t, const Grid &want) {
  REQUIRE(t.ncols() > 0);
  CHECK(t.ncols() == want[0].size());
  CHECK(t.nrows() == want.size());
  if (t.ncols() != want[0].size() || t.nrows() != want.size()) {
    return;
  }

  for (size_t r = 0; r < want.size(); ++r) {
    for (size_t c = 0; c < want[r].size(); ++c) {
      INFO("cell (" << r << "," << c << ")");
      CHECK(std::string(t.at(r, c)) == want[r][c]);
    }
  }
}

TEST_CASE("Quoted delimiter") {
  Table t = load("quoted_delimiter.csv");
  check_grid(t, {{"a", "b,c", "d"}, {"1", "2,3", "4"}});
}

TEST_CASE("Escaped quote") {
  Table t = load("escaped_quote.csv");
  check_grid(t, {{"a", "he said \"hi\"", "b"}});
}

TEST_CASE("new line inside quotes") {
  Table t = load("newline_in_quotes.csv");
  check_grid(t, {{"a", "line1\nline2", "c"}});
}

TEST_CASE("CRLF line ending") {
  Table t = load("crlf.csv");
  check_grid(t, {{"a", "b", "c"}, {"1", "2", "3"}});
  CHECK(t.at(0, 0).size() == 1);
}

TEST_CASE("Bom") {
  Table t = load("bom.csv");
  check_grid(t, {{"a", "b", "c"}, {"1", "2", "3"}});
  CHECK(t.at(0, 0).size() == 1);
}

TEST_CASE("Trailing delimiter") {
  Table t = load("trailing_delim.csv");
  check_grid(t, {{"a", "b", ""}, {"1", "2", ""}});
}

TEST_CASE("Ragged rows") {
  Table t = load("ragged.csv");
  check_grid(t, {{"a", "b", "c"}, {"1", "2", ""}, {"4", "5", "6"}});

  REQUIRE(t.errors.size() == 2);
  REQUIRE(t.errors[0].row == 1);
  REQUIRE(t.errors[0].kind == Table::error_kind::ShortRow);
  REQUIRE(t.errors[1].row == 2);
  REQUIRE(t.errors[1].kind == Table::error_kind::LongRow);
}

TEST_CASE("No trailing newline") {
  Table t = load("no_trailing_nl.csv");
  check_grid(t, {{"a", "b", "c"}, {"1", "2", "3"}});
}

TEST_CASE("Final short row") {
  Table t = load("final_short.csv");
  check_grid(t, {{"a", "b", "c"}, {"1", "2", ""}});

  REQUIRE(t.errors.size() == 1);
  REQUIRE(t.errors[0].row == 1);
  REQUIRE(t.errors[0].kind == Table::error_kind::ShortRow);
}

TEST_CASE("Final long row") {
  Table t = load("final_long.csv");
  check_grid(t, {{"a", "b", "c"}, {"1", "2", "3"}});

  REQUIRE(t.errors.size() == 1);
  REQUIRE(t.errors[0].row == 1);
  REQUIRE(t.errors[0].kind == Table::error_kind::LongRow);
}

TEST_CASE("Single line") {
  Table t = load("single_final.csv");
  check_grid(t, {{"a", "b", "c"}});
}

TEST_CASE("Quote mid field") {
  Table t = load("quote_mid_field.csv");
  check_grid(t, {{"ab\"cd", "e"}});
}

TEST_CASE("Quoted first field in a later row") {
  Table t = load("quoted_first_field.csv");
  check_grid(t, {{"name", "age"}, {"Smith, John", "42"}, {"Doe, Jane", "37"}});
  CHECK(t.errors.empty());
}

TEST_CASE("Quoted last field across serveral rows") {
  Table t = load("quoted_last_field.csv");
  check_grid(t, {{"a", "b", "c"}, {"1", "2", "3"}, {"4", "5", "6"}});
  CHECK(t.errors.empty());
}

TEST_CASE("Quoted last field with CRLF") {
  Table t = load("quoted_last_crlf.csv");
  check_grid(t, {{"a", "b", "c"}, {"1", "2", "3"}});
  CHECK(t.errors.empty());
}

TEST_CASE("Closed quote at EOF strips the quote") {
  Table t = load("quoted_at_eof.csv");
  check_grid(t, {{"a", "b", "c"}});
  CHECK(t.errors.empty());
}

TEST_CASE("Escaped field at EOF is unescaped") {
  Table t = load("escaped_at_eof.csv");
  check_grid(t, {{"a", "x\"y"}});
  CHECK(t.errors.empty());
}

TEST_CASE("Escaped field with CRLF is unescaped once") {
  Table t = load("escaped_crlf.csv");
  check_grid(t, {{"a", "x\"y"}});
  CHECK(t.errors.empty());
}

TEST_CASE("Trailing delimiter at EOF keeps the empty field") {
  Table t = load("trailing_delim_eof.csv");
  check_grid(t, {{"a", "b", "c"}, {"1", "2", ""}});
  CHECK(t.errors.empty());
}

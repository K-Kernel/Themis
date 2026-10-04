#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "helpers.hpp"
#include <doctest/doctest.h>
#include <themis/buffer.hpp>
#include <themis/io.hpp>
#include <themis/table.hpp>

using namespace themis;

TEST_CASE("Infer") {
  Table t = load("quoted_delimiter.csv");
  check_grid(t, {{"a", "b,c", "d"}, {"1", "2,3", "4"}});
}

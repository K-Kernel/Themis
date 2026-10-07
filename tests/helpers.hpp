#pragma once
#include <doctest/doctest.h>
#include <string>
#include <themis/io.hpp>
#include <themis/table.hpp>
#include <vector>

namespace themis {

using Grid = std::vector<std::vector<std::string>>;

inline Table load(const std::string &name, bool has_header = false) {
  return slice_csv(read_csv("data/corpus/" + name), has_header);
}

inline void check_grid(const Table &t, const Grid &want) {
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

inline void check_header(const Table &t, const std::vector<std::string> &want) {
  REQUIRE(t.header.size() == want.size());
  for (size_t i{0}; i < want.size(); ++i) {
    INFO("header (" << i << ")");
    CHECK(std::string(t.header[i]) == want[i]);
  }
}

} // namespace themis

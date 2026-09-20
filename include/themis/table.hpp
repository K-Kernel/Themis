#pragma once
#include <cstddef>
#include <string_view>
#include <themis/buffer.hpp>
#include <vector>

struct Table {
  Buffer data;
  std::vector<char> scratch;
  std::vector<std::string_view> header;
  std::vector<std::string_view> cells;
  size_t number_of_columns;
  enum error_kind { ShortRow, LongRow };
  struct parse_error {
    size_t row;
    size_t col;
    error_kind kind;
  };
  std::vector<parse_error> errors;

  size_t ncols() const { return number_of_columns; }
  size_t nrows() const {
    if (number_of_columns == 0) {
      return 0;
    };
    return cells.size() / number_of_columns;
  }

  std::string_view at(size_t row, size_t col) const {
    return cells[row * number_of_columns + col];
  };
};

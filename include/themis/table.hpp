#pragma once
#include "themis/buffer.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <variant>
#include <vector>

namespace themis {

using ColumnData = std::variant<std::vector<int64_t>, std::vector<double>,
                                std::vector<std::string_view>>;
struct Column {
  ColumnData data{}; // the variant's index is the type; don't store it twice
  std::vector<uint8_t> valid{}; // 1 = value, 0 = null
};

struct Table {
  themis::Buffer data{};
  std::vector<std::string_view> header{};
  std::vector<std::string_view> cells{};
  size_t number_of_columns{0};
  enum error_kind { ShortRow, LongRow, UnterminatedQuote };
  struct parse_error {
    size_t row;
    size_t col;
    error_kind kind;
  };
  std::vector<parse_error> errors{};
  std::shared_ptr<std::vector<char>> scratch{
      std::make_shared<std::vector<char>>()};
  std::vector<Column> columns{}; // empty until infer_types runs

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

  size_t col(std::string name) const {
    for (size_t i{0}; i < header.size(); ++i) {
      if (header[i] == name) {
        return i;
      }
    }
    throw std::out_of_range("Header not found");
  };
};
} // namespace themis

#pragma once
#include "themis/buffer.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
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

  size_t col(std::string_view name) const {
    for (size_t i{0}; i < header.size(); ++i) {
      if (header[i] == name) {
        return i;
      }
    }
    throw std::out_of_range("No column named" + std::string(name));
  };

  const Column &column(size_t col) const {
    if (columns.empty()) {
      throw std::logic_error("infer_types has not run");
    }
    return columns.at(col);
  }

  std::string column_name(size_t col) const {
    if (col < header.size()) {
      return std::string(header[col]);
    } else {
      return std::to_string(col);
    }
  }

  std::span<const int64_t> ints(size_t col) const & {
    const auto *v = std::get_if<std::vector<int64_t>>(&column(col).data);
    if (v == nullptr) {
      throw std::logic_error("column " + column_name(col) + " is not Int64");
    }
    return *v;
  }
  std::span<const int64_t> ints(size_t col) const && = delete;

  std::span<const double> doubles(size_t col) const & {
    const auto *v = std::get_if<std::vector<double>>(&column(col).data);
    if (v == nullptr) {
      throw std::logic_error("column " + column_name(col) + " is not Int64");
    }
    return *v;
  }
  std::span<const double> doubles(size_t col) const && = delete;

  std::span<const std::string_view> strings(size_t col) const & {
    const auto *v =
        std::get_if<std::vector<std::string_view>>(&column(col).data);
    if (v == nullptr) {
      throw std::logic_error("column " + column_name(col) + " is not Int64");
    }
    return *v;
  }
  std::span<const std::string> string(size_t col) const && = delete;

  std::vector<double> to_double(size_t col) {
    const Column &c = column(col);

    if (std::holds_alternative<std::vector<double>>(c.data)) {
      std::vector<double> copy = std::get<std::vector<double>>(c.data);
      return copy;
    } else if (std::holds_alternative<std::vector<std::int64_t>>(c.data)) {
      std::vector<std::int64_t> copy =
          std::get<std::vector<std::int64_t>>(c.data);
      std::vector<double> copy_d{};
      for (int64_t i : copy) {
        copy_d.push_back(static_cast<double>(i));
      }
      return copy_d;
    } else {
      throw std::logic_error("not numeric");
    }
  };
};
} // namespace themis

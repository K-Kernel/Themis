#pragma once

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <system_error>
#include <themis/table.hpp>
#include <utility>
#include <vector>

namespace themis {
namespace detail {

inline std::string_view trim(std::string_view sv) {

  while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t')) {
    sv.remove_prefix(1);
  }
  while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t')) {
    sv.remove_suffix(1);
  }
  return sv;
};

inline bool is_null(std::string_view sv) {
  sv = trim(sv);

  if (sv.empty()) {
    return true;
  }
  if (sv == "NA" || sv == "N/A" || sv == "n/a" || sv == "NaN") {
    return true;
  }
  if (sv == "null") {
    return true;
  }
  if (sv == "NULL") {
    return true;
  }
  if (sv == "None") {
    return true;
  }
  if (sv == "nan") {
    return true;
  }
  if (sv == "\\N") {
    return true;
  }
  return false;
};

inline bool parse_int(std::string_view sv, int64_t &number) {
  sv = trim(sv);

  if (sv.empty()) {
    return false;
  }

  if (sv.front() == '+') {
    sv.remove_prefix(1);
  }
  auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), number);
  return ec == std::errc{} && ptr == sv.data() + sv.size();
}

inline bool parse_double(std::string_view sv, double &number) {
  sv = trim(sv);
  if (sv.empty()) {
    return false;
  }
  if (sv.front() == '+') {
    sv.remove_prefix(1);
  }

  auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), number);
  return ec == std::errc{} && ptr == sv.data() + sv.size();
}
} // namespace detail

inline void infer_types(Table &table) {
  table.columns.clear();

  for (size_t c{0}; c < table.ncols(); ++c) {
    bool all_int = true;
    bool all_double = true;
    bool any_value = false;
    for (size_t r{0}; r < table.nrows(); ++r) {
      std::string_view cell = table.at(r, c);

      if (detail::is_null(table.at(r, c))) {
        continue;
      }
      any_value = true;

      int64_t i{};
      double d{};

      if (all_int && !detail::parse_int(cell, i)) {
        all_int = false;
      }
      if (all_double && !detail::parse_double(cell, d)) {
        all_double = false;
      }
    }
    Column col{};
    col.valid.assign(table.nrows(), 0);

    if (any_value && all_int) {
      std::vector<int64_t> out(table.nrows(), 0);
      for (size_t r{0}; r < table.nrows(); ++r) {
        std::string_view cell = table.at(r, c);
        if (!detail::is_null(cell)) {
          detail::parse_int(cell, out[r]);
          col.valid[r] = 1;
        }
      }
      col.data = std::move(out);
    } else if (any_value && all_double) {
      std::vector<double> out(table.nrows(), 0);
      for (size_t r{0}; r < table.nrows(); ++r) {
        std::string_view cell = table.at(r, c);
        if (!detail::is_null(cell)) {
          detail::parse_double(cell, out[r]);
          col.valid[r] = 1;
        }
      }
      col.data = std::move(out);
    } else {
      std::vector<std::string_view> out(table.nrows());
      for (size_t r{0}; r < table.nrows(); ++r) {
        std::string_view cell = table.at(r, c);
        out[r] = cell;
      }
      col.data = std::move(out);
    }
    table.columns.push_back(std::move(col));
  }
}
} // namespace themis

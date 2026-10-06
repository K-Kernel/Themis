#include <charconv>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <system_error>
#include <themis/table.hpp>

namespace themis {
namespace detail {

inline void trim(std::string_view &sv) {
  while (!sv.empty() && sv.front() == ' ') {
    sv.remove_prefix(1);
  }
  while (sv.empty() && sv.back() == ' ') {
    sv.remove_suffix(1);
  }
};

inline bool is_null(std::string_view sv) {
  trim(sv);

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

inline bool parse_int(std::string_view &sv, int64_t &number) {
  trim(sv);

  if (sv.empty()) {
    return false;
  }

  if (sv.front() == '+') {
    sv.remove_prefix(1);
  }
  auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), number);
  return ec == std::errc{} && ptr == sv.data() + sv.size();
}

inline bool parse_double(std::string_view &sv, double &number) {
  trim(sv);
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
  bool all_int = true;
  bool all_double = true;

  for (auto value : table.cells) {
    if (detail::is_null(value)) {
      continue;
    }

    if (all_int) {
      std::int64_t number;
      if (!detail::parse_int(value, number)) {
        all_int = false;
      };
    }

    if (!all_int && all_double) {
      std::double_t number;

      if (!detail::parse_double(value, number)) {
        all_double = false;
        break;
      }
    }
  }

  if (all_int) {

  } else if (all_double) {

  } else {
  }
}

} // namespace themis

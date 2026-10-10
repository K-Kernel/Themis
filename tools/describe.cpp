
// themis-describe [--no-header] file.csv
// Prints the shape of a CSV: size, parse errors, and one line per column.
#include "themis/io.hpp"
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

int main(int argc, char *argv[]) {
  // ---- 1. Arguments ----
  bool has_header = true;
  std::string path{};
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--no-header") {
      has_header = false;
    } else {
      path = arg;
    }
  }
  if (path.empty()) {
    std::cerr << "usage: themis-describe [--no-header] file.csv\n";
    return 2;
  }

  themis::Table t = themis::load_csv(path, has_header);

  // ---- 2. Size ----
  std::cout << t.nrows() << " rows x " << t.ncols() << " columns\n";

  // ---- 3. Parse errors: count per kind, and the first five rows of each ----
  const char *kind_names[] = {"ShortRow", "LongRow", "UnterminatedQuote"};
  for (int kind = 0; kind < 3; ++kind) {
    size_t count = 0;
    std::string rows;
    for (const auto &e : t.errors) {
      if (e.kind == kind) {
        if (count < 5) {
          rows += " " + std::to_string(e.row);
        }
        ++count;
      }
    }
    if (count > 0) {
      std::cout << "errors: " << kind_names[kind] << " x" << count << " (rows"
                << rows << ")\n";
    }
  }

  // ---- 4. One line per column ----
  const char *type_names[] = {"Int64", "Double", "String"};
  for (size_t c = 0; c < t.ncols(); ++c) {
    const themis::Column &col = t.column(c);
    size_t nulls =
        static_cast<size_t>(std::count(col.valid.begin(), col.valid.end(), 0));

    std::string summary;
    if (col.data.index() == 2) {
      // String: how many different values?
      std::unordered_set<std::string_view> distinct;
      auto values = t.strings(c);
      for (size_t r = 0; r < values.size(); ++r) {
        if (col.valid[r]) {
          distinct.insert(values[r]);
        }
      }
      summary = std::to_string(distinct.size()) + " distinct";
    } else {
      // Int64 or Double: smallest and largest real value.
      std::vector<double> values = t.to_doubles(c);
      bool any = false;
      double lo = 0, hi = 0;
      for (size_t r = 0; r < values.size(); ++r) {
        if (col.valid[r]) {
          if (!any || values[r] < lo)
            lo = values[r];
          if (!any || values[r] > hi)
            hi = values[r];
          any = true;
        }
      }
      if (any) {
        std::ostringstream range;
        range << "min " << lo << "  max " << hi;
        summary = range.str();
      }
    }

    std::string sample;
    for (size_t r = 0; r < t.nrows() && r < 3; ++r) {
      sample += " [" + std::string(t.at(r, c)) + "]";
    }

    std::cout << std::left << std::setw(14) << t.column_name(c) << std::setw(8)
              << type_names[col.data.index()] << "nulls " << std::setw(7)
              << nulls << std::setw(30) << summary << sample << "\n";
  }
}

#pragma once
#include <cstddef>
#include <themis/buffer.hpp>
#include <themis/table.hpp>
#include <vector>

inline Table slice_csv(Buffer csv) {
  std::string_view whole{csv.buffer_view.data(), csv.buffer_view.size()};

  // Check BOM
  if (whole.size() >= 3 && static_cast<unsigned char>(whole[0]) == 0xEF &&
      static_cast<unsigned char>(whole[1]) == 0xBB &&
      static_cast<unsigned char>(whole[2]) == 0xBF) {
    whole.remove_prefix(3);
  }

  // TODO:: Support header
  size_t ncols{0};
  std::vector<std::string_view> cells;
  bool first_record = true;
  enum class State { FieldStart, Quoted, QuoteInQuoted };
  State state{State::FieldStart};
  size_t field_start{0};

  for (size_t i{0}; i < whole.size(); ++i) {

    // TODO: Change this to a switch
    if (state == State::FieldStart) {
      if (whole[i] == ',') {
        cells.push_back(whole.substr(field_start, i - field_start));
        field_start = i + 1;
      }

      if (whole[i] == '\n') {
        if (whole[i - 1] == '\r') {
          cells.push_back(whole.substr(field_start, i - field_start - 1));
        } else {
          cells.push_back(whole.substr(field_start, i - field_start));
        }

        if (first_record) {
          ncols = cells.size();
          first_record = false;
        }

        field_start = i + 1;
      }

      if (whole[i] == '"') {
        field_start = i + 1;
        state = State::Quoted;
      }

    } else if (state == State::Quoted) {
      if (whole[i] == '"') {
        state = State::QuoteInQuoted;
      }
    } else if (state == State::QuoteInQuoted) {
      if (whole[i] == '"') {
        state = State::Quoted;
      } else {
        cells.push_back(whole.substr(field_start, i - field_start - 1));
        field_start = i + 1;
        state = State::FieldStart;
      }
    }
  }

  if (field_start < whole.size()) {
    cells.push_back(whole.substr(field_start, whole.size() - field_start));
  }

  return Table(csv, {}, cells, ncols);
}

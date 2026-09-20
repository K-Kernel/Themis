#pragma once
#include <cstddef>
#include <string_view>
#include <themis/buffer.hpp>
#include <themis/table.hpp>
#include <vector>

inline Table slice_csv(Buffer csv) {
  Table table{csv, {}, {}, {}, 0, {}};
  std::string_view whole{csv.buffer_view.data(), csv.buffer_view.size()};

  // Check BOM
  if (whole.size() >= 3 && static_cast<unsigned char>(whole[0]) == 0xEF &&
      static_cast<unsigned char>(whole[1]) == 0xBB &&
      static_cast<unsigned char>(whole[2]) == 0xBF) {
    whole.remove_prefix(3);
  }

  // TODO:: Support header

  size_t field_start{0};
  bool first_record = true;

  enum class State { FieldStart, Quoted, QuoteInQuoted };
  State state{State::FieldStart};

  table.scratch.reserve(whole.size());
  size_t scratch_field_start{0};
  bool using_scratch{false};
  size_t copy_start{0};

  size_t current_row_cells{0};
  // TODO: implement this
  size_t col{0};
  size_t row{0};

  for (size_t i{0}; i < whole.size(); ++i) {

    // TODO: Change this to a switch
    if (state == State::FieldStart) {
      if (whole[i] == ',') {
        table.cells.push_back(whole.substr(field_start, i - field_start));
        ++current_row_cells;
        field_start = i + 1;
      }

      if (whole[i] == '\n') {
        if (whole[i - 1] == '\r') {
          table.cells.push_back(whole.substr(field_start, i - field_start - 1));
          ++current_row_cells;

        } else {
          table.cells.push_back(whole.substr(field_start, i - field_start));
          ++current_row_cells;
        }

        if (first_record) {
          table.number_of_columns = current_row_cells;
          first_record = false;
        }

        if (current_row_cells < table.number_of_columns) {
          while (current_row_cells < table.number_of_columns) {
            table.cells.push_back("");
            ++current_row_cells;
          }
          table.errors.push_back({row, col, Table::ShortRow});
        } else if (current_row_cells > table.number_of_columns) {
          while (current_row_cells > table.number_of_columns) {
            table.cells.pop_back();
            --current_row_cells;
          }
          table.errors.push_back({row, col, Table::LongRow});
        };
        ++row;
        current_row_cells = 0;

        field_start = i + 1;
      }

      if (whole[i] == '"') {
        field_start = i + 1;
        copy_start = field_start;
        state = State::Quoted;
      }

    } else if (state == State::Quoted) {
      if (whole[i] == '"') {
        state = State::QuoteInQuoted;
      }

    } else if (state == State::QuoteInQuoted) {
      if (whole[i] == '"') {
        if (!using_scratch) {
          using_scratch = true;
          scratch_field_start = table.scratch.size();
        }

        table.scratch.insert(table.scratch.end(), whole.begin() + copy_start,
                             whole.begin() + i);
        copy_start = i + 1;
        state = State::Quoted;

      } else if (whole[i] == ',' || whole[i] == '\n') {

        if (using_scratch) {
          table.scratch.insert(table.scratch.end(), whole.begin() + copy_start,
                               whole.begin() + i - 1);

          std::string_view cell(table.scratch.data() + scratch_field_start,
                                table.scratch.size() - scratch_field_start);
          table.cells.push_back(cell);
          ++current_row_cells;
        } else {
          table.cells.push_back(whole.substr(field_start, i - field_start - 1));
          ++current_row_cells;
        }

        field_start = i + 1;
        state = State::FieldStart;
        using_scratch = false;
        continue;
      }
    }
  }

  if (field_start < whole.size()) {
    table.cells.push_back(
        whole.substr(field_start, whole.size() - field_start));
  }

  return table;
}

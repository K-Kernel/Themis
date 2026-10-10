#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <themis/buffer.hpp>
#include <themis/infer.hpp>
#include <themis/table.hpp>
#include <vector>

namespace themis {
namespace detail {

struct CsvCursor {
  std::string_view text;
  size_t field_start{0};
  size_t copy_start{0};
  size_t scratch_start{0};
  size_t row_cells{0};
  size_t row{0};
  bool first_record{true};
  bool in_scratch{false};
  bool has_header{false};
};

inline size_t line_end(std::string_view text, size_t i) {
  if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
    return 2;
  } else if (text[i] == '\r' or text[i] == '\n') {
    return 1;
  } else {
    return 0;
  }
};

inline void copy_to_scratch(Table &table, CsvCursor &cursor, size_t end) {
  if (!cursor.in_scratch) {
    cursor.in_scratch = true;
    cursor.scratch_start = table.scratch->size();
  }
  table.scratch->insert(table.scratch->end(),
                        cursor.text.begin() + cursor.copy_start,
                        cursor.text.begin() + end);
}

inline void emit_field(Table &table, CsvCursor &cursor, size_t end) {
  if (cursor.in_scratch) {
    copy_to_scratch(table, cursor, end);
    std::string_view cell(table.scratch->data() + cursor.scratch_start,
                          table.scratch->size() - cursor.scratch_start);
    table.cells.push_back(cell);
    cursor.in_scratch = false;
  } else {
    std::string_view cell =
        cursor.text.substr(cursor.field_start, end - cursor.field_start);
    table.cells.push_back(cell);
  }
  ++cursor.row_cells;
}

inline void emit_record(Table &table, CsvCursor &cursor) {
  if (cursor.first_record) {
    table.number_of_columns = cursor.row_cells;
    cursor.first_record = false;

    if (cursor.has_header) {
      table.header = table.cells;
      table.cells.clear();
      for (auto &name : table.header) {
        name = trim(name);
      }
      cursor.row_cells = 0;
      return;
    }
  }

  if (cursor.row_cells < table.number_of_columns) {
    table.errors.push_back({cursor.row, cursor.row_cells, Table::ShortRow});
    while (cursor.row_cells < table.number_of_columns) {
      table.cells.push_back("");
      ++cursor.row_cells;
    }

  } else if (cursor.row_cells > table.number_of_columns) {
    table.errors.push_back(
        {cursor.row, table.number_of_columns, Table::LongRow});
    while (cursor.row_cells > table.number_of_columns) {
      table.cells.pop_back();
      --cursor.row_cells;
    }
  }

  ++cursor.row;
  cursor.row_cells = 0;
}

} // namespace detail
inline Table slice_csv(Buffer csv, bool has_header) {
  Table table{.data = csv};
  detail::CsvCursor cursor{
      .text{std::string_view{csv.buffer_view.data(), csv.buffer_view.size()}},
      .has_header = has_header};

  // Check BOM
  if (cursor.text.size() >= 3 &&
      static_cast<unsigned char>(cursor.text[0]) == 0xEF &&
      static_cast<unsigned char>(cursor.text[1]) == 0xBB &&
      static_cast<unsigned char>(cursor.text[2]) == 0xBF) {
    cursor.text.remove_prefix(3);
  }

  enum class State { FieldStart, Unquoted, Quoted, QuoteInQuoted };
  State state{State::FieldStart};

  table.scratch->reserve(cursor.text.size());

  for (size_t i{0}; i < cursor.text.size(); ++i) {

    // TODO: Change this to a switch
    if (state == State::Quoted) {
      if (cursor.text[i] == '"') {
        state = State::QuoteInQuoted;
      }
      continue;
    } else {
      size_t eol = detail::line_end(cursor.text, i);
      if (eol != 0) {
        if (state == State::FieldStart) {
          if (cursor.row_cells > 0) {
            detail::emit_field(table, cursor, i);
            detail::emit_record(table, cursor);
          }
        } else if (state == State::Unquoted) {
          detail::emit_field(table, cursor, i);
          detail::emit_record(table, cursor);
        } else if (state == State::QuoteInQuoted) {
          detail::emit_field(table, cursor, i - 1);
          detail::emit_record(table, cursor);
        }
        cursor.field_start = i + eol;
        i += eol - 1;
        state = State::FieldStart;
        continue;
      }
    }

    switch (state) {
    case State::FieldStart:
      if (cursor.text[i] == '"') {
        cursor.field_start = i + 1;
        cursor.copy_start = i + 1;
        state = State::Quoted;
      } else if (cursor.text[i] == ',') {
        detail::emit_field(table, cursor, i);
        cursor.field_start = i + 1;
      } else {
        state = State::Unquoted;
      }
      break;

    case State::Unquoted:
      if (cursor.text[i] == ',') {
        detail::emit_field(table, cursor, i);
        cursor.field_start = i + 1;
        state = State::FieldStart;
      }
      break;

    case State::QuoteInQuoted:
      if (cursor.text[i] == '"') {
        detail::copy_to_scratch(table, cursor, i);
        cursor.copy_start = i + 1;
        state = State::Quoted;
      } else if (cursor.text[i] == ',') {
        detail::emit_field(table, cursor, i - 1);
        cursor.field_start = i + 1;
        state = State::FieldStart;
      } else {
        detail::copy_to_scratch(table, cursor, i - 1);
        cursor.copy_start = i;
        state = State::Unquoted;
      }
      break;

    case State::Quoted:
      break;
    }
  }

  // EOF handling
  if (state == State::FieldStart && cursor.row_cells > 0) {
    detail::emit_field(table, cursor, cursor.text.size());
    detail::emit_record(table, cursor);
  } else if (state == State::Unquoted) {
    detail::emit_field(table, cursor, cursor.text.size());
    detail::emit_record(table, cursor);
  } else if (state == State::Quoted) {
    table.errors.push_back(
        {cursor.row, cursor.row_cells, Table::UnterminatedQuote});
    detail::emit_field(table, cursor, cursor.text.size());
    detail::emit_record(table, cursor);
  } else if (state == State::QuoteInQuoted) {
    detail::emit_field(table, cursor, cursor.text.size() - 1);
    detail::emit_record(table, cursor);
  }
  return table;
}

inline Table load_csv(const std::string &path, bool has_header = true) {
  Table t = slice_csv(read_csv(path), has_header);
  infer_types(t);
  return t;
}

} // namespace themis

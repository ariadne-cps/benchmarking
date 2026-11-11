/***************************************************************************
 *            benchmarking/table.hpp
 *
 *  Copyright  2026  Luca Geretti
 *
 ****************************************************************************/

/*
 *  This file is part of Ariadne Benchmarking.
 *
 *  Ariadne Benchmarking is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Ariadne Benchmarking is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne Benchmarking. If not, see <https://www.gnu.org/licenses/>.
 */

/** @file benchmarking/table.hpp
 *  @brief Generic dynamic-width tables for benchmark output.
 */

#ifndef ARIADNE_BENCHMARKING_TABLE_HPP
#define ARIADNE_BENCHMARKING_TABLE_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Ariadne::Benchmarking {

//! Select horizontal alignment of a column.
enum class TableAlignment { Left,
                            Right };

// The actual width is max(minimum_width, heading length, every rendered value).
// With the default minimum_width=0, no width is imposed arbitrarily.
//! Describe a column's heading, optional minimum width and alignment.
struct TableColumn {
    std::string heading;
    std::size_t minimum_width = 0u;
    TableAlignment alignment = TableAlignment::Left;
};

//! Configure cell separator, optional horizontal rules and grouping indices.
//! Rows are displayed in insertion order, not sorted by group.
struct TableOptions {
    std::string separator = "  ";
    char top_rule = '\0';
    char header_rule = '\0';
    char group_rule = '\0';
    char bottom_rule = '\0';
    // Draw a separator whenever any selected column changes between rows.
    // Keys are zero-based column indices; rows are not reordered.
    std::vector<std::size_t> group_columns;

    //! Return an option set drawing top, heading, group and bottom rules.
    static TableOptions ruled() {
        TableOptions options;
        options.top_rule = '=';
        options.header_rule = '-';
        options.group_rule = '-';
        options.bottom_rule = '=';
        return options;
    }
};

// A generic table for benchmark measurements AND cross-implementation
// comparisons. Values are formatted before computing column widths.
//! Render newline-free textual cells with widths chosen from formatted content.
class Table {
  public:
    //! Create a table; validate column count, group indices and separator characters.
    explicit Table(std::vector<TableColumn> columns,
                   TableOptions options = {})
        : _columns(std::move(columns)), _options(std::move(options)) {
        if (_columns.empty())
            throw std::invalid_argument("a table requires columns");
        for (const auto& column: _columns)
            validate_cell(column.heading);
        for (auto index: _options.group_columns) {
            if (index >= _columns.size())
                throw std::invalid_argument("invalid table group column");
        }
        if (_options.separator.find_first_of("\r\n") != std::string::npos)
            throw std::invalid_argument("table separator cannot contain a newline");
        for (char symbol: {_options.top_rule, _options.header_rule,
                           _options.group_rule, _options.bottom_rule}) {
            if (symbol == '\r' or symbol == '\n')
                throw std::invalid_argument("invalid table rule character");
        }
    }

    //! Append a row matching the declared number of columns.
    void add_row(std::vector<std::string> values) {
        if (values.size() != _columns.size())
            throw std::invalid_argument("incorrect number of table values");
        for (const auto& value: values)
            validate_cell(value);
        _rows.push_back(std::move(values));
    }

    //! Render headings and rows with computed widths, alignment and rules.
    std::string str() const {
        // First format all cells, then measure the *displayed strings*.
        std::vector<std::size_t> widths;
        widths.reserve(_columns.size());
        for (const auto& column: _columns)
            widths.push_back(std::max(column.minimum_width, column.heading.size()));
        for (const auto& row: _rows) {
            for (std::size_t i = 0u; i < row.size(); ++i)
                widths[i] = std::max(widths[i], row[i].size());
        }

        const std::size_t spacing = _options.separator.size();
        std::size_t width = 0u;
        for (std::size_t i = 0u; i < widths.size(); ++i) {
            if (widths[i] > std::numeric_limits<std::size_t>::max() - width)
                throw std::overflow_error("table width overflow");
            width += widths[i];
            if (i != 0u) {
                if (spacing > std::numeric_limits<std::size_t>::max() - width)
                    throw std::overflow_error("table separator width overflow");
                width += spacing;
            }
        }

        std::ostringstream output;
        bool first_line = true;
        auto append_line = [&](const std::string& line) {
            if (not first_line)
                output << '\n';
            output << line;
            first_line = false;
        };
        auto rule = [&](char symbol) {
            if (symbol != '\0')
                append_line(std::string(width, symbol));
        };
        auto render = [&](const std::vector<std::string>& values) {
            std::string line;
            line.reserve(width);
            for (std::size_t i = 0u; i < values.size(); ++i) {
                if (i != 0u)
                    line += _options.separator;
                const std::size_t pad = widths[i] - values[i].size();
                if (_columns[i].alignment == TableAlignment::Right)
                    line.append(pad, ' ');
                line += values[i];
                if (_columns[i].alignment == TableAlignment::Left)
                    line.append(pad, ' ');
            }
            append_line(line);
        };

        rule(_options.top_rule);
        std::vector<std::string> headings;
        headings.reserve(_columns.size());
        for (const auto& column: _columns)
            headings.push_back(column.heading);
        render(headings);
        rule(_options.header_rule);
        for (std::size_t i = 0u; i < _rows.size(); ++i) {
            if (i != 0u and _options.group_rule != '\0') {
                bool changed = false;
                for (auto index: _options.group_columns) {
                    if (_rows[i - 1u][index] != _rows[i][index]) {
                        changed = true;
                        break;
                    }
                }
                if (changed)
                    rule(_options.group_rule);
            }
            render(_rows[i]);
        }
        rule(_options.bottom_rule);
        return output.str();
    }

    //! Format a finite value to fixed decimal precision, or n/a for nonfinite values.
    //! @throws std::invalid_argument if digits exceeds 30.
    static std::string number(double value, unsigned digits = 3u,
                              std::string suffix = {}) {
        if (digits > 30u)
            throw std::invalid_argument("table precision is too high");
        if (not std::isfinite(value))
            return "n/a";
        std::ostringstream output;
        output << std::fixed << std::setprecision(static_cast<int>(digits))
               << value << suffix;
        return output.str();
    }

  private:
    static void validate_cell(const std::string& value) {
        if (value.find_first_of("\r\n") != std::string::npos)
            throw std::invalid_argument("table values must be single-line");
    }

    std::vector<TableColumn> _columns;
    TableOptions _options;
    std::vector<std::vector<std::string>> _rows;
};

} // namespace Ariadne::Benchmarking

#endif

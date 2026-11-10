/***************************************************************************
 *            benchmarking/reporting.hpp
 *
 *  Copyright  2026  Luca Geretti
 *
 ****************************************************************************/

/*
 *  This file is part of Ariadne.
 *
 *  Ariadne is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Ariadne is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne.  If not, see <https://www.gnu.org/licenses/>.
 */

/*! \file benchmarking/reporting.hpp
 *  \brief Generate summary tables from measured benchmark statistics.
 */

#ifndef ARIADNE_BENCHMARKING_REPORTING_HPP
#define ARIADNE_BENCHMARKING_REPORTING_HPP

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "benchmarking/statistics.hpp"
#include "benchmarking/summary_fields.hpp"
#include "benchmarking/table.hpp"
#include "logging/logging.hpp"

namespace Ariadne::Benchmarking {

//! Display unit for benchmark statistics; input measurements remain ns.
enum class ReportTimeUnit {
    Nanoseconds,
    Microseconds,
    Milliseconds,
    Seconds
};

// Display-only conversion from internally stored nanoseconds.
inline double report_time_divisor(ReportTimeUnit unit) {
    switch (unit) {
        case ReportTimeUnit::Nanoseconds:
            return 1.0;
        case ReportTimeUnit::Microseconds:
            return 1e3;
        case ReportTimeUnit::Milliseconds:
            return 1e6;
        case ReportTimeUnit::Seconds:
            return 1e9;
        default:
            throw std::invalid_argument("invalid report time unit");
    }
}

inline std::string_view report_time_suffix(ReportTimeUnit unit) {
    switch (unit) {
        case ReportTimeUnit::Nanoseconds:
            return "ns";
        case ReportTimeUnit::Microseconds:
            return "us";
        case ReportTimeUnit::Milliseconds:
            return "ms";
        case ReportTimeUnit::Seconds:
            return "s";
        default:
            throw std::invalid_argument("invalid report time unit");
    }
}

//! Own statistics rows and render configurable adaptive-width summary tables.
//! Formatting is separate from timing.
class Report {
  public:
    //! Construct a named report with a selected ordered set of statistic columns.
    explicit Report(std::string title, SummaryFields columns = default_summary_fields())
        : _title(std::move(title)) {
        set_summary_fields(std::move(columns));
        _dimensions = {
            {"Configuration", 0u, TableAlignment::Left},
            {    "Operation", 0u, TableAlignment::Left}
        };
    }

    //! Set the optional parameter description line printed above the table.
    void set_parameter_header(std::string header) {
        _parameter_header = std::move(header);
    }

    //! Select distinct, nonempty statistic columns in display order.
    void set_summary_fields(SummaryFields fields) {
        if (fields.empty())
            throw std::invalid_argument("summary columns cannot be empty");
        SummaryFields checked;
        for (SummaryField field: fields) {
            (void)summary_field_id(field);
            if (std::find(checked.begin(), checked.end(), field) != checked.end())
                throw std::invalid_argument("duplicate summary column");
            checked.push_back(field);
        }
        _columns = std::move(checked);
    }

    // Use arbitrary descriptive dimensions (e.g. Backend, Operation, Bits).
    // Their widths and the numeric columns are computed from rendered values.
    //! Configure descriptive columns; existing row widths must agree.
    void set_dimension_columns(std::vector<TableColumn> dimensions) {
        if (dimensions.empty())
            throw std::invalid_argument("report needs descriptive columns");
        for (const auto& row: _rows) {
            if (row.dimensions.size() != dimensions.size())
                throw std::invalid_argument("existing rows have different dimensions");
        }
        _dimensions = std::move(dimensions);
    }

    //! Set grouping, separators and optional table rules.
    void set_table_options(TableOptions options) {
        _options = std::move(options);
    }

    //! Return the ordered selected statistics fields.
    const SummaryFields& summary_fields() const { return _columns; }

    //! Only change the report presentation; Runner and Samples still use ns.
    Void set_time_unit(ReportTimeUnit unit) {
        (void)report_time_divisor(unit); // Reject invalid enum values.
        _time_unit = unit;
    }
    ReportTimeUnit time_unit() const { return _time_unit; }

    // Keep the original Report API for existing benchmark clients.
    //! Append a row through the legacy configuration/operation API.
    void add(std::string configuration, std::string operation, Statistics result) {
        add_row({std::move(configuration), std::move(operation)}, result);
    }

    //! Append statistics with one value per descriptive dimension.
    void add_row(std::vector<std::string> dimensions, Statistics result) {
        if (dimensions.size() != _dimensions.size())
            throw std::invalid_argument("incorrect report dimension count");
        _rows.push_back({std::move(dimensions), result});
    }

    // Pure formatting, separately testable without changing Logger settings.
    //! Render the report as a string without altering logger settings.
    std::string table() const {
        std::vector<TableColumn> columns = _dimensions;
        for (SummaryField field: _columns) {
            std::string heading(summary_field_heading(field));
            if (field != SummaryField::Count) {
                const auto pos = heading.find("(ns");
                if (pos != std::string::npos)
                    heading.replace(pos, 3u, "(" + std::string(report_time_suffix(_time_unit)));
            }
            columns.push_back({std::move(heading), 0u, TableAlignment::Right});
        }

        TableOptions options = _options;
        for (auto index: options.group_columns) {
            if (index >= _dimensions.size())
                throw std::invalid_argument("report grouping needs a dimension");
        }

        Table table(std::move(columns), std::move(options));
        for (const auto& row: _rows) {
            std::vector<std::string> cells = row.dimensions;
            for (SummaryField field: _columns)
                cells.push_back(formatted_value(row.result, field, _time_unit));
            table.add_row(std::move(cells));
        }
        return table.str();
    }

    //! Log the report at the current level (verbosity one at the root).
    void print() const {
        LOGGING_PRINTLN(_title)
        if (not _parameter_header.empty())
            LOGGING_PRINTLN(_parameter_header)
        LOGGING_PRINTLN(table())
    }

  private:
    struct Row {
        std::vector<std::string> dimensions;
        Statistics result;
    };

    static std::string formatted_value(
        const Statistics& stats, SummaryField field, ReportTimeUnit time_unit) {
        if (field == SummaryField::Count)
            return std::to_string(stats.count);
        // set_summary_fields() validates every field before storing it.
        // The remaining enum values index the timing statistics in enum order.
        constexpr double Statistics::* members[] = {
            &Statistics::minimum, &Statistics::maximum, &Statistics::mean,
            &Statistics::mad, &Statistics::median, &Statistics::p95,
            &Statistics::stddev, &Statistics::variance};
        static_assert(std::size(members) == static_cast<std::size_t>(SummaryField::Count));
        double value = stats.*members[static_cast<std::size_t>(field)];
        // Variance is in squared time units, all other timing fields
        // have first-order units. Sample counts are never scaled.
        const double divisor = report_time_divisor(time_unit);
        value /= field == SummaryField::Variance ? divisor * divisor : divisor;
        return Table::number(value);
    }

    std::string _title;
    std::string _parameter_header;
    SummaryFields _columns;
    ReportTimeUnit _time_unit = ReportTimeUnit::Nanoseconds;
    std::vector<TableColumn> _dimensions;
    TableOptions _options;
    std::vector<Row> _rows;
};

} // namespace Ariadne::Benchmarking

#endif

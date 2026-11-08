#ifndef ARIADNE_BENCHMARKING_REPORTING_HPP
#define ARIADNE_BENCHMARKING_REPORTING_HPP

#include "benchmarking/statistics.hpp"
#include "benchmarking/summary_fields.hpp"
#include "benchmarking/table.hpp"
#include "logging/logging.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Ariadne::Benchmarking {

class Report {
public:
    explicit Report(std::string title, SummaryFields columns = default_summary_fields())
        : _title(std::move(title)) {
        set_summary_fields(std::move(columns));
        _dimensions = {
            {"Configuration", 0u, TableAlignment::Left},
            {"Operation", 0u, TableAlignment::Left}
        };
    }

    void set_parameter_header(std::string header) {
        _parameter_header = std::move(header);
    }

    void set_summary_fields(SummaryFields fields) {
        if (fields.empty())
            throw std::invalid_argument("summary columns cannot be empty");
        SummaryFields checked;
        for (SummaryField field : fields) {
            (void)summary_field_id(field);
            if (std::find(checked.begin(), checked.end(), field) != checked.end())
                throw std::invalid_argument("duplicate summary column");
            checked.push_back(field);
        }
        _columns = std::move(checked);
    }

    // Use arbitrary descriptive dimensions (e.g. Backend, Operation, Bits).
    // Their widths and the numeric columns are computed from rendered values.
    void set_dimension_columns(std::vector<TableColumn> dimensions) {
        if (dimensions.empty())
            throw std::invalid_argument("report needs descriptive columns");
        for (auto const& row : _rows) {
            if (row.dimensions.size() != dimensions.size())
                throw std::invalid_argument("existing rows have different dimensions");
        }
        _dimensions = std::move(dimensions);
    }

    void set_table_options(TableOptions options) {
        _options = std::move(options);
    }

    SummaryFields const& summary_fields() const { return _columns; }

    // Keep the original Report API for existing benchmark clients.
    void add(std::string configuration, std::string operation, Statistics result) {
        add_row({std::move(configuration), std::move(operation)}, result);
    }

    void add_row(std::vector<std::string> dimensions, Statistics result) {
        if (dimensions.size() != _dimensions.size())
            throw std::invalid_argument("incorrect report dimension count");
        _rows.push_back({std::move(dimensions), result});
    }

    // Pure formatting, separately testable without changing Logger settings.
    std::string table() const {
        std::vector<TableColumn> columns = _dimensions;
        for (SummaryField field : _columns)
            columns.push_back({std::string(summary_field_heading(field)),
                               0u, TableAlignment::Right});

        TableOptions options = _options;
        for (auto index : options.group_columns) {
            if (index >= _dimensions.size())
                throw std::invalid_argument("report grouping needs a dimension");
        }

        Table table(std::move(columns), std::move(options));
        for (auto const& row : _rows) {
            std::vector<std::string> cells = row.dimensions;
            for (SummaryField field : _columns)
                cells.push_back(formatted_value(row.result, field));
            table.add_row(std::move(cells));
        }
        return table.str();
    }

    void print() const {
        LOGGING_PRINTLN_AT(1, _title)
        if (not _parameter_header.empty())
            LOGGING_PRINTLN_AT(1, _parameter_header)
        LOGGING_PRINTLN_AT(1, table())
    }

private:
    struct Row {
        std::vector<std::string> dimensions;
        Statistics result;
    };

    static std::string formatted_value(Statistics const& stats, SummaryField field) {
        if (field == SummaryField::Count) return std::to_string(stats.count);
        double value = 0.0;
        switch (field) {
            case SummaryField::Minimum: value = stats.minimum; break;
            case SummaryField::Maximum: value = stats.maximum; break;
            case SummaryField::Mean: value = stats.mean; break;
            case SummaryField::MAD: value = stats.mad; break;
            case SummaryField::Median: value = stats.median; break;
            case SummaryField::P95: value = stats.p95; break;
            case SummaryField::Stddev: value = stats.stddev; break;
            case SummaryField::Variance: value = stats.variance; break;
            case SummaryField::Count: break;
            default: throw std::invalid_argument("unknown summary field");
        }
        return Table::number(value);
    }

    std::string _title;
    std::string _parameter_header;
    SummaryFields _columns;
    std::vector<TableColumn> _dimensions;
    TableOptions _options;
    std::vector<Row> _rows;
};

} // namespace Ariadne::Benchmarking

#endif

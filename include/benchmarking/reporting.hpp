#ifndef ARIADNE_BENCHMARKING_REPORTING_HPP
#define ARIADNE_BENCHMARKING_REPORTING_HPP

#include "benchmarking/statistics.hpp"
#include "benchmarking/summary_fields.hpp"
#include "logging/logging.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
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
    }

    void set_parameter_header(std::string header) {
        _parameter_header = std::move(header);
    }

    void set_summary_fields(SummaryFields fields) {
        if (fields.empty()) throw std::invalid_argument("summary columns cannot be empty");
        SummaryFields checked;
        for (SummaryField const field : fields) {
            (void)summary_field_id(field);
            if (std::find(checked.begin(), checked.end(), field) != checked.end())
                throw std::invalid_argument("duplicate summary column");
            checked.push_back(field);
        }
        _columns = std::move(checked);
    }

    SummaryFields const& summary_fields() const { return _columns; }

    void add(std::string case_name, std::string operation, Statistics result) {
        _rows.push_back({std::move(case_name), std::move(operation), result});
    }

    // Pure formatting is independently testable without redirecting Logger.
    std::string table() const {
        std::size_t first_width = std::string("Configuration").size();
        for (auto const& row : _rows)
            first_width = std::max(first_width, row.case_name.size());
        first_width += 2u;

        std::ostringstream out;
        out << std::left << std::setw(static_cast<int>(first_width)) << "Configuration"
            << std::setw(24) << "Operation";
        for (SummaryField field : _columns)
            out << std::right << std::setw(static_cast<int>(column_width(field)))
                << summary_field_heading(field);
        for (auto const& row : _rows) {
            out << '\n' << std::left
                << std::setw(static_cast<int>(first_width)) << row.case_name
                << std::setw(24) << row.operation;
            for (SummaryField field : _columns)
                out << std::right << std::setw(static_cast<int>(column_width(field)))
                    << formatted_value(row.result, field);
        }
        return out.str();
    }

    void print() const {
        LOGGING_PRINTLN_AT(1, _title)
        if (not _parameter_header.empty())
            LOGGING_PRINTLN_AT(1, _parameter_header)
        LOGGING_PRINTLN_AT(1, table())
    }

private:
    struct Row {
        std::string case_name;
        std::string operation;
        Statistics result;
    };

    static std::size_t column_width(SummaryField field) {
        return std::max<std::size_t>(
            field == SummaryField::Count ? 8u : 14u,
            summary_field_heading(field).size() + 2u);
    }

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
        }
        // Sample variance is undefined with fewer than two measurements.
        if (not std::isfinite(value)) return "n/a";
        std::ostringstream formatted;
        formatted << std::fixed << std::setprecision(3) << value;
        return formatted.str();
    }

    std::string _title;
    std::string _parameter_header;
    SummaryFields _columns;
    std::vector<Row> _rows;
};
} // namespace Ariadne::Benchmarking

#endif

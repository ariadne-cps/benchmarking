#ifndef ARIADNE_BENCHMARKING_REPORTING_HPP
#define ARIADNE_BENCHMARKING_REPORTING_HPP

#include "benchmarking/statistics.hpp"
#include "logging/logging.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Ariadne::Benchmarking {

class Report {
public:
    explicit Report(std::string title) : _title(std::move(title)) {}

    void set_parameter_header(std::string header) { _parameter_header = std::move(header); }

    void add(std::string case_name, std::string operation,
             Statistics result) {
        _rows.push_back({std::move(case_name), std::move(operation), result});
    }

    void print() const {
        LOGGING_PRINTLN_AT(1, _title)
        if (not _parameter_header.empty())
            LOGGING_PRINTLN_AT(1, _parameter_header)
        std::size_t first_width = std::string("Configuration").size();
        for (auto const& row : _rows)
            first_width = std::max(first_width, row.case_name.size());
        first_width += 2u;
        LOGGING_PRINTLN_AT(1, std::left << std::setw(static_cast<int>(first_width)) << "Configuration"
                        << std::setw(24) << "Operation"
                        << std::right << std::setw(14) << "Median (ns)"
                        << std::setw(14) << "P95 (ns)"
                        << std::setw(8) << "N")
        for (auto const& row : _rows) {
            LOGGING_PRINTLN_AT(1, std::left << std::setw(static_cast<int>(first_width)) << row.case_name
                            << std::setw(24) << row.operation
                            << std::right << std::fixed << std::setprecision(3)
                            << std::setw(14) << row.result.median
                            << std::setw(14) << row.result.p95
                            << std::setw(8) << row.result.count)
        }
    }

private:
    struct Row {
        std::string case_name;
        std::string operation;
        Statistics result;
    };
    std::string _title;
    std::string _parameter_header;
    std::vector<Row> _rows;
};
} // namespace Ariadne::Benchmarking

#endif

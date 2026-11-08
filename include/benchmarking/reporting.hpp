#ifndef ARIADNE_BENCHMARKING_REPORTING_HPP
#define ARIADNE_BENCHMARKING_REPORTING_HPP

#include "benchmarking/statistics.hpp"
#include "logging/logging.hpp"
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Ariadne::Benchmarking {

class Report {
public:
    explicit Report(std::string title) : _title(std::move(title)) {}

    void add(std::string case_name, std::string operation,
             Statistics result) {
        _rows.push_back({std::move(case_name), std::move(operation), result});
    }

    void print() const {
        LOGGING_PRINTLN(_title)
        LOGGING_PRINTLN(std::left << std::setw(24) << "Case"
                        << std::setw(24) << "Operation"
                        << std::right << std::setw(14) << "Median (ns)"
                        << std::setw(14) << "P95 (ns)"
                        << std::setw(8) << "N")
        for (auto const& row : _rows) {
            LOGGING_PRINTLN(std::left << std::setw(24) << row.case_name
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
    std::vector<Row> _rows;
};
} // namespace Ariadne::Benchmarking

#endif

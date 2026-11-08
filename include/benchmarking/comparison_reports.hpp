#ifndef ARIADNE_BENCHMARKING_COMPARISON_REPORTS_HPP
#define ARIADNE_BENCHMARKING_COMPARISON_REPORTS_HPP

#include "benchmarking/comparison_results.hpp"
#include "benchmarking/table.hpp"
#include "logging/logging.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Ariadne::Benchmarking {

enum class ComparisonReportKind { Measurements, Pairs, Aggregates };

struct ComparisonReportDefinition {
    std::string name;
    ComparisonReportKind kind = ComparisonReportKind::Measurements;
    std::vector<std::string> columns;
    TableOptions options;
    // Pair and aggregate ratios are derived from the same location metric.
    SummaryField metric = SummaryField::Median;
    // For aggregates: false gives one aggregate per candidate over all cases.
    bool by_configuration = true;
};

// Multiple report definitions may render the *same* benchmark results with
// different columns, grouping, metrics and aggregation levels. The definitions
// never influence which measurements are executed or retained.
class ComparisonReports {
public:
    void add(ComparisonReportDefinition definition) {
        if (definition.name.empty() or definition.columns.empty())
            throw std::invalid_argument("report name and columns are mandatory");
        for (auto const& existing : _definitions) {
            if (existing.name == definition.name)
                throw std::invalid_argument("duplicate comparison report name");
        }
        for (std::size_t i = 0u; i < definition.columns.size(); ++i) {
            if (definition.columns[i].empty())
                throw std::invalid_argument("empty report column");
            if (std::find(definition.columns.begin(),
                          definition.columns.begin() + static_cast<std::ptrdiff_t>(i),
                          definition.columns[i]) !=
                definition.columns.begin() + static_cast<std::ptrdiff_t>(i))
                throw std::invalid_argument("duplicate report column");
        }
        _definitions.push_back(std::move(definition));
    }

    static ComparisonReports standard() {
        ComparisonReports reports;
        reports.add({"measurements", ComparisonReportKind::Measurements,
            {"implementation","operation","configuration","median"}});
        reports.add({"pairs", ComparisonReportKind::Pairs,
            {"candidate","operation","configuration",
             "reference_ns","candidate_ns","slowdown"}});
        reports.add({"aggregates", ComparisonReportKind::Aggregates,
            {"candidate","configuration","operations",
             "geomean","total_time","worst","worst_operation"}});
        return reports;
    }

    std::string render(std::string const& name,
                       ComparisonResults const& results,
                       std::string const& reference) const {
        for (auto const& definition : _definitions) {
            if (definition.name == name)
                return render_definition(definition, results, reference);
        }
        throw std::invalid_argument("unknown report name");
    }

    std::string render_all(ComparisonResults const& results,
                           std::string const& reference) const {
        std::string output;
        for (auto const& definition : _definitions) {
            if (not output.empty()) output += "\n\n";
            output += definition.name + "\n";
            output += render_definition(definition, results, reference);
        }
        return output;
    }

    void print_all(ComparisonResults const& results,
                   std::string const& reference) const {
        LOGGING_PRINTLN_AT(1, render_all(results, reference))
    }

private:
    using Cells = std::map<std::string, std::string>;

    static std::string heading(std::string const& name) {
        static std::map<std::string, std::string> const headings{
            {"implementation","Implementation"},{"operation","Operation"},
            {"configuration","Configuration"},{"reference","Reference"},
            {"candidate","Candidate"},
            {"reference_ns","Reference (ns)"},{"candidate_ns","Candidate (ns)"},
            {"slowdown","Slowdown (x)"},{"speedup","Speedup (x)"},
            {"operations","Operations"},{"geomean","Geomean (x)"},
            {"total_time","Total time (x)"},{"worst","Worst (x)"},
            {"worst_operation","Worst operation"},
            {"worst_configuration","Worst configuration"}};
        auto const found = headings.find(name);
        if (found != headings.end()) return found->second;
        if (name.compare(0u, 6u, "param:") == 0)
            return name.substr(6u);
        for (SummaryField field : {SummaryField::Minimum,SummaryField::Maximum,
                                  SummaryField::Mean,SummaryField::MAD,
                                  SummaryField::Median,SummaryField::P95,
                                  SummaryField::Stddev,SummaryField::Variance,
                                  SummaryField::Count}) {
            if (name == summary_field_id(field))
                return std::string(summary_field_heading(field));
        }
        return name;
    }

    static void dimensions(Cells& cells, ComparisonContext const& context) {
        cells.emplace("configuration", context.label);
        for (auto const& [name, value] : context.dimensions)
            cells.emplace("param:" + name, value);
    }

    static void statistics(Cells& cells, Statistics const& stats) {
        cells.emplace("min", Table::number(stats.minimum));
        cells.emplace("max", Table::number(stats.maximum));
        cells.emplace("mean", Table::number(stats.mean));
        cells.emplace("mad", Table::number(stats.mad));
        cells.emplace("median", Table::number(stats.median));
        cells.emplace("p95", Table::number(stats.p95));
        cells.emplace("stddev", Table::number(stats.stddev));
        cells.emplace("variance", Table::number(stats.variance));
        cells.emplace("count", std::to_string(stats.count));
    }

    static bool numeric(std::string const& field) {
        return field == "min" or field == "max" or field == "mean" or
               field == "mad" or field == "median" or field == "p95" or
               field == "stddev" or field == "variance" or field == "count" or
               field == "operations" or field == "reference_ns" or
               field == "candidate_ns" or field == "slowdown" or
               field == "speedup" or field == "geomean" or
               field == "total_time" or field == "worst";
    }

    static std::string render_definition(ComparisonReportDefinition const& def,
                                         ComparisonResults const& results,
                                         std::string const& reference) {
        std::vector<Cells> data;
        if (def.kind == ComparisonReportKind::Measurements) {
            for (auto const& measurement : results.measurements()) {
                Cells row;
                row.emplace("implementation", measurement.implementation);
                row.emplace("operation", measurement.operation);
                dimensions(row, measurement.context);
                statistics(row, measurement.statistics);
                data.push_back(std::move(row));
            }
        } else if (def.kind == ComparisonReportKind::Pairs) {
            for (auto const& pair : results.paired(reference, def.metric)) {
                Cells row;
                row.emplace("reference", pair.reference);
                row.emplace("candidate", pair.candidate);
                row.emplace("operation", pair.operation);
                dimensions(row, pair.context);
                row.emplace("reference_ns", Table::number(pair.reference_ns));
                row.emplace("candidate_ns", Table::number(pair.candidate_ns));
                row.emplace("slowdown", Table::number(pair.slowdown));
                row.emplace("speedup", Table::number(pair.speedup));
                data.push_back(std::move(row));
            }
        } else {
            for (auto const& aggregate : results.aggregates(
                     reference, def.by_configuration, def.metric)) {
                Cells row;
                row.emplace("reference", aggregate.reference);
                row.emplace("candidate", aggregate.candidate);
                row.emplace("configuration", aggregate.configuration);
                row.emplace("operations", std::to_string(aggregate.operations));
                row.emplace("geomean", Table::number(aggregate.geomean));
                row.emplace("total_time", Table::number(aggregate.total_time));
                row.emplace("worst", Table::number(aggregate.worst));
                row.emplace("worst_operation", aggregate.worst_operation);
                row.emplace("worst_configuration", aggregate.worst_configuration);
                data.push_back(std::move(row));
            }
        }
        // Validate column names against the view's schema, even when no
        // measurements exist, instead of silently printing empty cells.
        Cells allowed;
        if (def.kind == ComparisonReportKind::Measurements) {
            for (auto const& field : {"implementation","operation","configuration",
                "min","max","mean","mad","median","p95","stddev","variance","count"})
                allowed.emplace(field,"");
        } else if (def.kind == ComparisonReportKind::Pairs) {
            for (auto const& field : {"reference","candidate","operation",
                                     "configuration","reference_ns",
                                     "candidate_ns","slowdown","speedup"})
                allowed.emplace(field,"");
        } else {
            for (auto const& field : {"reference","candidate","configuration",
                "operations","geomean","total_time","worst",
                "worst_operation","worst_configuration"})
                allowed.emplace(field,"");
        }

        std::vector<TableColumn> columns;
        for (auto const& field : def.columns) {
            if (field.compare(0u, 6u, "param:") == 0) {
                if (def.kind == ComparisonReportKind::Aggregates)
                    throw std::invalid_argument(
                        "aggregate views cannot select a parameter dimension");
                // Empty collections cannot establish available dimensions.
                if (data.empty())
                    throw std::invalid_argument("no measurements for dimension selection");
                for (auto const& row : data) {
                    if (row.find(field) == row.end())
                        throw std::invalid_argument("missing report parameter: " + field);
                }
            } else if (allowed.find(field) == allowed.end()) {
                throw std::invalid_argument("unsupported report column: " + field);
            }
            columns.push_back({heading(field),0u,
                               numeric(field) ? TableAlignment::Right
                                              : TableAlignment::Left});
        }

        Table table(std::move(columns), def.options);
        for (auto const& item : data) {
            std::vector<std::string> cells;
            cells.reserve(def.columns.size());
            for (auto const& field : def.columns)
                cells.push_back(item.at(field));
            table.add_row(std::move(cells));
        }
        return table.str();
    }

    std::vector<ComparisonReportDefinition> _definitions;
};

} // namespace Ariadne::Benchmarking

#endif

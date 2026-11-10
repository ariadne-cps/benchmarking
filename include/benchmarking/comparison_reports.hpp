/***************************************************************************
 *            benchmarking/comparison_reports.hpp
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

/*! \file benchmarking/comparison_reports.hpp
 *  \brief Configurable tabular views of comparison results.
 */

#ifndef ARIADNE_BENCHMARKING_COMPARISON_REPORTS_HPP
#define ARIADNE_BENCHMARKING_COMPARISON_REPORTS_HPP

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "benchmarking/comparison_results.hpp"
#include "benchmarking/table.hpp"
#include "logging/logging.hpp"

namespace Ariadne::Benchmarking {

//! Select raw measurements, paired ratios or aggregated ratios.
enum class ComparisonReportKind { Measurements,
                                  Pairs,
                                  Aggregates };

//! Describe one independently configured, named view and its column order.
struct ComparisonReportDefinition {
    std::string name;
    ComparisonReportKind kind = ComparisonReportKind::Measurements;
    std::vector<std::string> columns;
    TableOptions options;
    // Pair and aggregate ratios are derived from the same location metric.
    //! Time-location statistic used for paired and aggregate timing ratios.
    SummaryField metric = SummaryField::Median;
    // For aggregates: false gives one aggregate per candidate over all cases.
    //! Whether to aggregate separately per configuration.
    bool by_configuration = true;

    //! Construct a named view with a kind and selected column identifiers.
    ComparisonReportDefinition(std::string report_name,
                               ComparisonReportKind report_kind,
                               std::vector<std::string> selected_columns)
        : name(std::move(report_name)), kind(report_kind),
          columns(std::move(selected_columns)) {}
};

// Multiple report definitions may render the *same* benchmark results with
// different columns, grouping, metrics and aggregation levels. The definitions
// never influence which measurements are executed or retained.
//! Render any number of views over the same retained measurements.
class ComparisonReports {
  public:
    //! Add a view, rejecting missing names or columns and duplicates.
    void add(ComparisonReportDefinition definition) {
        if (definition.name.empty() or definition.columns.empty())
            throw std::invalid_argument("report name and columns are mandatory");
        for (const auto& existing: _definitions) {
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

    //! Return standard raw, paired and aggregate view definitions.
    static ComparisonReports standard() {
        ComparisonReports reports;
        reports.add({
            "measurements", ComparisonReportKind::Measurements, {"implementation", "operation", "configuration", "median"}
        });
        reports.add({
            "pairs", ComparisonReportKind::Pairs, {"candidate", "operation", "configuration", "reference_ns", "candidate_ns", "slowdown"}
        });
        reports.add({
            "aggregates", ComparisonReportKind::Aggregates, {"candidate", "configuration", "operations", "geomean", "total_time", "worst", "worst_operation"}
        });
        return reports;
    }

    //! Render one named view against a chosen reference implementation.
    std::string render(const std::string& name,
                       const ComparisonResults& results,
                       const std::string& reference) const {
        for (const auto& definition: _definitions) {
            if (definition.name == name)
                return render_definition(definition, results, reference);
        }
        throw std::invalid_argument("unknown report name");
    }

    //! Render each configured view, separated by blank lines.
    std::string render_all(const ComparisonResults& results,
                           const std::string& reference) const {
        std::string output;
        for (const auto& definition: _definitions) {
            if (not output.empty())
                output += "\n\n";
            output += definition.name + "\n";
            output += render_definition(definition, results, reference);
        }
        return output;
    }

    //! Write all rendered views at the current Logging level.
    //! At the root scope this is visible with --verbosity 1.
    void print_all(const ComparisonResults& results,
                   const std::string& reference) const {
        LOGGING_PRINTLN(render_all(results, reference))
    }

  private:
    using Cells = std::map<std::string, std::string>;

    static std::string heading(const std::string& name) {
        static const std::map<std::string, std::string> headings{
            {     "implementation",      "Implementation"},
            {          "operation",           "Operation"},
            {      "configuration",       "Configuration"},
            {          "reference",           "Reference"},
            {          "candidate",           "Candidate"},
            {       "reference_ns",      "Reference (ns)"},
            {       "candidate_ns",      "Candidate (ns)"},
            {           "slowdown",        "Slowdown (x)"},
            {            "speedup",         "Speedup (x)"},
            {         "operations",          "Operations"},
            {            "geomean",         "Geomean (x)"},
            {         "total_time",      "Total time (x)"},
            {              "worst",           "Worst (x)"},
            {    "worst_operation",     "Worst operation"},
            {"worst_configuration", "Worst configuration"}
        };
        const auto found = headings.find(name);
        if (found != headings.end())
            return found->second;
        if (name.compare(0u, 6u, "param:") == 0)
            return name.substr(6u);
        // render_definition() validates the schema before requesting headings.
        // Remaining names are summary fields; reuse their parser and labels.
        return std::string(summary_field_heading(parse_summary_fields(name).front()));
    }

    static void dimensions(Cells& cells, const ComparisonContext& context) {
        cells.emplace("configuration", context.label);
        for (const auto& [name, value]: context.dimensions)
            cells.emplace("param:" + name, value);
    }

    static void statistics(Cells& cells, const Statistics& stats) {
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

    static bool numeric(const std::string& field) {
        return field == "min" or field == "max" or field == "mean" or
               field == "mad" or field == "median" or field == "p95" or
               field == "stddev" or field == "variance" or field == "count" or
               field == "operations" or field == "reference_ns" or
               field == "candidate_ns" or field == "slowdown" or
               field == "speedup" or field == "geomean" or
               field == "total_time" or field == "worst";
    }

    static std::string render_definition(const ComparisonReportDefinition& def,
                                         const ComparisonResults& results,
                                         const std::string& reference) {
        std::vector<Cells> data;
        if (def.kind == ComparisonReportKind::Measurements) {
            for (const auto& measurement: results.measurements()) {
                Cells row;
                row.emplace("implementation", measurement.implementation);
                row.emplace("operation", measurement.operation);
                dimensions(row, measurement.context);
                statistics(row, measurement.statistics);
                data.push_back(std::move(row));
            }
        } else if (def.kind == ComparisonReportKind::Pairs) {
            for (const auto& pair: results.paired(reference, def.metric)) {
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
            for (const auto& aggregate: results.aggregates(
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
            for (const auto& field: {"implementation", "operation", "configuration",
                                     "min", "max", "mean", "mad", "median", "p95", "stddev", "variance", "count"})
                allowed.emplace(field, "");
        } else if (def.kind == ComparisonReportKind::Pairs) {
            for (const auto& field: {"reference", "candidate", "operation",
                                     "configuration", "reference_ns",
                                     "candidate_ns", "slowdown", "speedup"})
                allowed.emplace(field, "");
        } else {
            for (const auto& field: {"reference", "candidate", "configuration",
                                     "operations", "geomean", "total_time", "worst",
                                     "worst_operation", "worst_configuration"})
                allowed.emplace(field, "");
        }

        std::vector<TableColumn> columns;
        for (const auto& field: def.columns) {
            if (field.compare(0u, 6u, "param:") == 0) {
                if (def.kind == ComparisonReportKind::Aggregates)
                    throw std::invalid_argument(
                        "aggregate views cannot select a parameter dimension");
                // Empty collections cannot establish available dimensions.
                if (data.empty())
                    throw std::invalid_argument("no measurements for dimension selection");
                for (const auto& row: data) {
                    if (row.find(field) == row.end())
                        throw std::invalid_argument("missing report parameter: " + field);
                }
            } else if (allowed.find(field) == allowed.end()) {
                throw std::invalid_argument("unsupported report column: " + field);
            }
            columns.push_back({heading(field), 0u,
                               numeric(field) ? TableAlignment::Right
                                              : TableAlignment::Left});
        }

        Table table(std::move(columns), def.options);
        for (const auto& item: data) {
            std::vector<std::string> cells;
            cells.reserve(def.columns.size());
            for (const auto& field: def.columns)
                cells.push_back(item.at(field));
            table.add_row(std::move(cells));
        }
        return table.str();
    }

    std::vector<ComparisonReportDefinition> _definitions;
};

} // namespace Ariadne::Benchmarking

#endif

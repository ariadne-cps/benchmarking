/***************************************************************************
 *            benchmarking/comparison_results.hpp
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

/*! \file benchmarking/comparison_results.hpp
 *  \brief Store, pair and aggregate cross-implementation timing data.
 */

#ifndef ARIADNE_BENCHMARKING_COMPARISON_RESULTS_HPP
#define ARIADNE_BENCHMARKING_COMPARISON_RESULTS_HPP

#include "benchmarking/parameters.hpp"
#include "benchmarking/samples.hpp"
#include "benchmarking/statistics.hpp"
#include "benchmarking/summary_fields.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Ariadne::Benchmarking {

//! Identify one configuration by ordinal, label and named parameter dimensions.
struct ComparisonContext {
    std::size_t ordinal = 1u;
    std::string label = "default";
    std::map<std::string, std::string> dimensions;

    //! Capture an immutable display context from a configuration.
    static ComparisonContext from(Configuration const& configuration) {
        ComparisonContext result;
        result.ordinal = configuration.ordinal();
        result.label = configuration.label().empty()
            ? "default" : configuration.label();
        for (auto const& [name, value] : configuration.dimensions())
            result.dimensions.emplace(name, value);
        return result;
    }
};

//! One completed measurement with its implementation and operation key.
struct MeasurementRow {
    std::string implementation;
    std::string operation;
    ComparisonContext context;
    Statistics statistics;
};

//! One reference/candidate pair: slowdown = candidate/reference, speedup = reference/candidate.
struct ComparisonRow {
    std::string reference;
    std::string candidate;
    std::string operation;
    ComparisonContext context;
    double reference_ns = 0.0;
    double candidate_ns = 0.0;
    double slowdown = 0.0;  // candidate / reference: > 1 means slower
    double speedup = 0.0;   // reference / candidate: > 1 means faster
};

//! Aggregate slowdowns: geometric mean, summed-time ratio and worst individual case.
struct AggregateRow {
    std::string reference;
    std::string candidate;
    std::string configuration;
    std::size_t operations = 0u;
    double geomean = 0.0;
    double total_time = 0.0;
    double worst = 0.0;
    std::string worst_operation;
    std::string worst_configuration;
};

// Owns completed measurements, not measured callables. A comparison is
// considered valid only when all registered candidates contain exactly the
// same (configuration ordinal, operation) keys as the reference.
//! Own completed statistics and enforce identical operation/configuration keys across implementations.
//! Configuration ordinals identify cases even when displayed parameter values repeat.
class ComparisonResults {
public:
    //! Register a nonempty, unique implementation name.
    void register_implementation(std::string name) {
        if (name.empty())
            throw std::invalid_argument("implementation name cannot be empty");
        if (std::find(_implementations.begin(), _implementations.end(), name)
            != _implementations.end())
            throw std::invalid_argument("duplicate implementation name");
        _implementations.push_back(std::move(name));
    }

    //! Return registered implementations in insertion order.
    std::vector<std::string> const& implementations() const {
        return _implementations;
    }

    //! Append samples or precomputed statistics for a unique operation/configuration/implementation key.
    //! @throws std::invalid_argument for unknown implementations, duplicates or inconsistent metadata.
    void add(std::string implementation, std::string operation,
             ComparisonContext context, Samples const& samples) {
        add(std::move(implementation), std::move(operation),
            std::move(context), summarize(samples));
    }

    //! Append samples or precomputed statistics for a unique operation/configuration/implementation key.
    //! @throws std::invalid_argument for unknown implementations, duplicates or inconsistent metadata.
    void add(std::string implementation, std::string operation,
             ComparisonContext context, Statistics statistics) {
        if (std::find(_implementations.begin(), _implementations.end(),
                      implementation) == _implementations.end())
            throw std::invalid_argument("unregistered implementation");
        if (operation.empty() or context.ordinal == 0u)
            throw std::invalid_argument("invalid comparison key");
        for (auto const& row : _rows) {
            if (row.context.ordinal == context.ordinal and
                (row.context.label != context.label or
                 row.context.dimensions != context.dimensions))
                throw std::invalid_argument(
                    "conflicting metadata for the same configuration ordinal");
            if (row.context.ordinal == context.ordinal and
                row.operation == operation and
                row.implementation == implementation)
                throw std::invalid_argument("duplicate benchmark measurement");
        }
        _rows.push_back({std::move(implementation), std::move(operation),
                         std::move(context), statistics});
    }

    //! Return retained measurement rows.
    std::vector<MeasurementRow> const& measurements() const { return _rows; }

    // Only elapsed-time location statistics have meaningful ratio semantics;
    // dispersion and count must not be used as the timing metric.
    //! Extract min, max, mean, median or p95 for timing ratios; reject dispersion/count.
    static double timing(Statistics const& stats, SummaryField metric) {
        switch (metric) {
            case SummaryField::Minimum: return stats.minimum;
            case SummaryField::Maximum: return stats.maximum;
            case SummaryField::Mean: return stats.mean;
            case SummaryField::Median: return stats.median;
            case SummaryField::P95: return stats.p95;
            default: throw std::invalid_argument(
                "comparison metric must be min,max,mean,median or p95");
        }
    }

    //! Match candidate and reference measurements by ordinal and operation.
    //! @param metric Time-location statistic to compare.
    //! @throws std::invalid_argument on missing keys or nonpositive/nonfinite measurements.
    std::vector<ComparisonRow> paired(
            std::string const& reference,
            SummaryField metric = SummaryField::Median) const {
        if (std::find(_implementations.begin(), _implementations.end(),
                      reference) == _implementations.end())
            throw std::invalid_argument("reference implementation is not registered");
        if (_implementations.size() < 2u)
            throw std::invalid_argument("a comparison needs at least two implementations");

        using Key = std::pair<std::size_t, std::string>;
        std::map<Key, MeasurementRow const*> references;
        for (auto const& row : _rows) {
            if (row.implementation == reference)
                references.emplace(Key(row.context.ordinal, row.operation), &row);
        }
        if (references.empty())
            throw std::invalid_argument("no reference measurements");

        std::vector<ComparisonRow> result;
        for (auto const& candidate : _implementations) {
            if (candidate == reference) continue;
            std::map<Key, MeasurementRow const*> candidates;
            for (auto const& row : _rows) {
                if (row.implementation == candidate)
                    candidates.emplace(Key(row.context.ordinal, row.operation), &row);
            }
            if (candidates.size() != references.size())
                throw std::invalid_argument(
                    "unpaired comparison: different operation counts");
            for (auto const& row : _rows) {
                if (row.implementation != reference) continue;
                auto const found = candidates.find(
                    Key(row.context.ordinal, row.operation));
                if (found == candidates.end())
                    throw std::invalid_argument(
                        "unpaired comparison: missing operation or configuration");
                MeasurementRow const& other = *found->second;
                if (row.context.label != other.context.label or
                    row.context.dimensions != other.context.dimensions)
                    throw std::invalid_argument("mismatched comparison metadata");
                double const ref_ns = timing(row.statistics, metric);
                double const cand_ns = timing(other.statistics, metric);
                if (not std::isfinite(ref_ns) or not std::isfinite(cand_ns)
                    or ref_ns <= 0.0 or cand_ns <= 0.0)
                    throw std::invalid_argument("comparison timings must be positive and finite");
                double const slowdown = cand_ns / ref_ns;
                double const speedup = ref_ns / cand_ns;
                if (not std::isfinite(slowdown) or not std::isfinite(speedup)
                    or slowdown <= 0.0 or speedup <= 0.0)
                    throw std::overflow_error("comparison ratio overflow");
                result.push_back({reference, candidate, row.operation, row.context,
                                  ref_ns, cand_ns, slowdown, speedup});
            }
        }
        return result;
    }

    //! Summarize paired slowdowns per configuration or globally.
    //! @param by_configuration When false, aggregate over every configuration.
    std::vector<AggregateRow> aggregates(
            std::string const& reference,
            bool by_configuration = true,
            SummaryField metric = SummaryField::Median) const {
        struct Accumulation {
            AggregateRow row;
            double log_sum = 0.0;
            double reference_sum = 0.0;
            double candidate_sum = 0.0;
        };
        std::vector<Accumulation> groups;
        std::map<std::pair<std::string, std::size_t>, std::size_t> positions;
        for (auto const& pair : paired(reference, metric)) {
            std::size_t const ordinal =
                by_configuration ? pair.context.ordinal : 0u;
            auto const key = std::make_pair(pair.candidate, ordinal);
            auto position = positions.find(key);
            if (position == positions.end()) {
                std::size_t const index = groups.size();
                positions.emplace(key, index);
                Accumulation group{};
                group.row.reference = reference;
                group.row.candidate = pair.candidate;
                group.row.configuration = by_configuration
                    ? pair.context.label : "all";
                groups.push_back(std::move(group));
                position = positions.find(key);
            }
            Accumulation& group = groups[position->second];
            ++group.row.operations;
            group.log_sum += std::log(pair.slowdown);
            group.reference_sum += pair.reference_ns;
            group.candidate_sum += pair.candidate_ns;
            if (pair.slowdown > group.row.worst) {
                group.row.worst = pair.slowdown;
                group.row.worst_operation = pair.operation;
                group.row.worst_configuration = pair.context.label;
            }
        }
        std::vector<AggregateRow> result;
        result.reserve(groups.size());
        for (auto& group : groups) {
            if (not std::isfinite(group.reference_sum) or
                not std::isfinite(group.candidate_sum) or
                group.reference_sum <= 0.0 or group.candidate_sum <= 0.0)
                throw std::overflow_error("aggregate timing overflow");
            group.row.geomean = std::exp(group.log_sum /
                static_cast<double>(group.row.operations));
            group.row.total_time = group.candidate_sum / group.reference_sum;
            if (not std::isfinite(group.row.geomean) or
                not std::isfinite(group.row.total_time))
                throw std::overflow_error("aggregate ratio overflow");
            result.push_back(std::move(group.row));
        }
        return result;
    }

private:
    std::vector<std::string> _implementations;
    std::vector<MeasurementRow> _rows;
};

} // namespace Ariadne::Benchmarking

#endif

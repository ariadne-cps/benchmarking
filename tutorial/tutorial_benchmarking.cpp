/***************************************************************************
 *            tutorial_benchmarking.cpp
 *
 *  Copyright  2026
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

#include "ariadne-benchmarking.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace Ariadne;
namespace Bench = Ariadne::Benchmarking;


// Part II runs after the parameterized sum experiment, using the same CLI
// but its own fixed configuration and five measured rounds per backend.
// The two implementations are intentionally equivalent. In a Release build
// they may compile into identical machine instructions: the point is to
// demonstrate typed dispatch, not a meaningful performance ranking.
// Part II: minimal A/B, two type-based implementations of one operation.
struct VariantA {
    static unsigned increment(unsigned value) { return value + 1u; }
};
struct VariantB {
    static unsigned increment(unsigned value) { return 1u + value; }
};

static void run_ab_comparison(Bench::CommandLine const& cli) {
    // An empty Matrix represents exactly one implicit configuration.
    // Unlike Part I, no lengths/iterations/samples command-line axis applies.
    Bench::Matrix one_configuration;
    Bench::Runner runner(cli);
    // The battery instantiates the workload once per concrete type. The
    // implementation call within the timed loop needs no virtual dispatch.
    Bench::ComparisonBattery<VariantA, VariantB> battery({"A", "B"});

    // record registers one operation for this Implementation. Its callbacks
    // are synchronous: value remains alive through preparation and checking.
    auto workload = []<class Implementation>(auto&& record) {
        unsigned value = 0u;
        // Each full execution contains 1,000 increments; normalization later
        // divides the elapsed duration by 1,000 to obtain ns/increment.
        record("increment",
            [&] { value = 0u; },
            [&] {
                for (unsigned i = 0u; i < 1000u; ++i) {
                    value = Implementation::increment(value);
                    Bench::keep_alive(value);
                }
            },
            [&] {
                if (value != 1000u)
                    throw std::runtime_error("incorrect increment result");
            },
            1000.0);
    };
    // Five measured rounds per type, plus any global --warmups. The
    // --samples parameter from Part I does not control this comparison.
    battery.run(one_configuration, runner, 5u, workload);

    // The default paired timing metric is median. The displayed slowdown
    // is candidate/reference: values > 1 indicate that B is slower than A.
    // The global --summary option does not change this view's columns.
    Bench::ComparisonReports reports;
    reports.add({"A/B", Bench::ComparisonReportKind::Pairs,
                 {"operation", "reference_ns", "candidate_ns", "slowdown"}});
    reports.print_all(battery.results(), "A");
}

int main(int argc, char const* argv[]) {
    try {
        Bench::Parameter<unsigned> lengths("lengths", "Input lengths", {64u,256u});
        Bench::Parameter<unsigned> iterations("iterations", "Iterations", {1000u});
        Bench::Parameter<unsigned> samples_count("samples", "Samples", {7u});
        // Axes are borrowed (not owned) by Matrix; keep the Parameter
        // objects alive throughout CLI parsing and lazy iteration.
        // With 2 lengths, 2 iteration counts and 1 sample count, there
        // are four configurations; the last registered axis varies fastest.
        Bench::Matrix matrix;
        matrix.add(lengths).add(iterations).add(samples_count);
        Bench::CommandLine cli;
        cli.add_parameters(matrix);
        // Parse all global and axis-specific options before measuring.
        // A non-execution CLI request, such as help, exits successfully.
        if (not cli.acquire(Bench::arguments(argc, argv))) return 0;

        // Demonstrate advanced report layout in the non-comparison tutorial.
        // Report takes already-computed Statistics; it never times work.
        // --summary controls which numeric statistics follow the four
        // descriptive columns below, and in what order.
        Bench::Report report("Ariadne Benchmarking tutorial", cli.summary_fields());
        report.set_dimension_columns({
            {"Length", 0u, Bench::TableAlignment::Right},
            {"Iterations", 0u, Bench::TableAlignment::Right},
            {"Samples", 0u, Bench::TableAlignment::Right},
            {"Operation"}
        });
        Bench::TableOptions layout = Bench::TableOptions::ruled();
        // Insert a separating rule when adjacent rows change Length.
        // Column widths come from rendered content, not fixed padding.
        layout.group_columns = {0u};
        report.set_table_options(layout);
        report.set_parameter_header(matrix.header());
        // The matrix size determines log depth; --warmups is a global
        // policy inherited from CommandLine, not an experiment axis.
        Bench::Runner runner(cli, matrix.combinations());
        matrix.for_each([&](Bench::Configuration const& config) {
            runner.configuration_started(config);
            // Configuration resolves the selected typed values. Length is
            // vector size, iterations is traversals per timed operation,
            // and samples is the number of reported timing observations.
            unsigned const size = config.value(lengths);
            unsigned const repetitions = config.value(iterations);
            unsigned const measured = config.value(samples_count);
            if (size == 0u || repetitions == 0u || measured == 0u)
                throw std::invalid_argument("lengths, iterations and samples must be positive");
            // Initialize the vector only once per configuration. Its sum
            // is the correctness oracle for every measured/warm-up round.
            // Input construction and oracle computation are outside timing.
            // Prepare the input before starting the timer.
            std::vector<std::uint64_t> data(size);
            std::uint64_t expected = 0u;
            for (std::size_t i = 0u; i < data.size(); ++i) {
                data[i] = static_cast<std::uint64_t>(i + 1u);
                expected += data[i];
            }
            std::uint64_t sum = 0u;
            // Every round is prepare -> timed operation -> verify. Warm-ups
            // follow the same sequence but do not enter Samples. The final
            // argument divides full-run nanoseconds by repetitions, so a
            // sample reports ns per *vector traversal*, not per element.
            Bench::Samples results = runner.run(
                "sum", measured,
                [&] {
                    // Reset mutable state; otherwise each sample would
                    // inherit the previous round's accumulated sum.
                    // Called before EVERY round, including warm-ups.
                    sum = 0u;
                },
                [&] {
                    for (unsigned k = 0u; k < repetitions; ++k) {
                        for (auto const value : data) sum += value;
                        // Make the accumulator observable so the compiler
                        // cannot simply delete unused work in a timed loop.
                        Bench::keep_alive(sum);
                    }
                },
                [&] {
                    // Reject incorrect results, including warm-ups. This
                    // validation does not inflate the timed duration.
                    // Verify every execution outside the timer.
                    if (sum != expected * repetitions)
                        throw std::runtime_error("incorrect sum");
                },
                static_cast<double>(repetitions));
            // Summarize only the retained, normalized measured samples.
            // Each row carries the concrete descriptive matrix values.
            report.add_row({
                std::to_string(size), std::to_string(repetitions),
                std::to_string(measured), "sum"
            }, Bench::summarize(results));
            runner.configuration_completed(config);
        });
        // Rendering and Logging run after all measurements, not in the
        // timed callback. The report prints the parameter header and table.
        report.print();

        // Part II: A/B follows the complete non-A/B example.
        run_ab_comparison(cli);
        return 0;
    } catch (std::exception const& error) {
        LOGGING_PRINTLN("tutorial_benchmarking: " << error.what())
        return 1;
    }
}

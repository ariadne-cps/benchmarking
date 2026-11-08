#include "ariadne-benchmarking.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace Ariadne;
namespace Bench = Ariadne::Benchmarking;


// Part II: minimal A/B, two type-based implementations of one operation.
struct VariantA {
    static unsigned increment(unsigned value) { return value + 1u; }
};
struct VariantB {
    static unsigned increment(unsigned value) { return 1u + value; }
};

static void run_ab_comparison(Bench::CommandLine const& cli) {
    Bench::Matrix one_configuration;
    Bench::Runner runner(cli);
    Bench::ComparisonBattery<VariantA, VariantB> battery({"A", "B"});

    auto workload = []<class Implementation>(auto&& record) {
        unsigned value = 0u;
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
    battery.run(one_configuration, runner, 5u, workload);

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
        Bench::Matrix matrix;
        matrix.add(lengths).add(iterations).add(samples_count);
        Bench::CommandLine cli;
        cli.add_parameters(matrix);
        if (not cli.acquire(Bench::arguments(argc, argv))) return 0;

        // Demonstrate advanced report layout in the non-comparison tutorial.
        Bench::Report report("Ariadne Benchmarking tutorial", cli.summary_fields());
        report.set_dimension_columns({
            {"Length", 0u, Bench::TableAlignment::Right},
            {"Iterations", 0u, Bench::TableAlignment::Right},
            {"Samples", 0u, Bench::TableAlignment::Right},
            {"Operation"}
        });
        Bench::TableOptions layout = Bench::TableOptions::ruled();
        layout.group_columns = {0u};
        report.set_table_options(layout);
        report.set_parameter_header(matrix.header());
        Bench::Runner runner(cli, matrix.combinations());
        matrix.for_each([&](Bench::Configuration const& config) {
            runner.configuration_started(config);
            unsigned const size = config.value(lengths);
            unsigned const repetitions = config.value(iterations);
            unsigned const measured = config.value(samples_count);
            if (size == 0u || repetitions == 0u || measured == 0u)
                throw std::invalid_argument("lengths, iterations and samples must be positive");
            // Prepare the input before starting the timer.
            std::vector<std::uint64_t> data(size);
            std::uint64_t expected = 0u;
            for (std::size_t i = 0u; i < data.size(); ++i) {
                data[i] = static_cast<std::uint64_t>(i + 1u);
                expected += data[i];
            }
            std::uint64_t sum = 0u;
            Bench::Samples results = runner.run(
                "sum", measured,
                [&] {
                    // Called before EVERY round, including warm-ups.
                    sum = 0u;
                },
                [&] {
                    for (unsigned k = 0u; k < repetitions; ++k) {
                        for (auto const value : data) sum += value;
                        Bench::keep_alive(sum);
                    }
                },
                [&] {
                    // Verify every execution outside the timer.
                    if (sum != expected * repetitions)
                        throw std::runtime_error("incorrect sum");
                },
                static_cast<double>(repetitions));
            report.add_row({
                std::to_string(size), std::to_string(repetitions),
                std::to_string(measured), "sum"
            }, Bench::summarize(results));
            runner.configuration_completed(config);
        });
        report.print();

        // Part II: A/B follows the complete non-A/B example.
        run_ab_comparison(cli);
        return 0;
    } catch (std::exception const& error) {
        LOGGING_PRINTLN("tutorial_benchmarking: " << error.what())
        return 1;
    }
}

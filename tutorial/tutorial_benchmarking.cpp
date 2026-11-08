#include "ariadne-benchmarking.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace Ariadne;
namespace Bench = Ariadne::Benchmarking;

int main(int argc, char const* argv[]) {
    try {
        Bench::Parameter<unsigned> lengths("lengths", "Input lengths", {64u,256u});
        Bench::Parameter<unsigned> iterations("iterations", "Iterations", {1000u});
        Bench::Parameter<unsigned> samples_count("samples", "Samples", {7u});
        Bench::Parameter<unsigned> warmups("warmups", "Warmup rounds", {2u});
        Bench::Matrix matrix;
        matrix.add(lengths).add(iterations).add(samples_count).add(warmups);
        Bench::CommandLine cli;
        cli.add_parameters(matrix);
        if (not cli.acquire(Bench::arguments(argc, argv))) return 0;

        Bench::Report report("Ariadne Benchmarking tutorial", cli.summary_fields());
        report.set_parameter_header(matrix.header());
        Bench::RunLog log(matrix.combinations());
        matrix.for_each([&](Bench::Configuration const& config) {
            log.configuration_started(config);
            unsigned const size = config.value(lengths);
            unsigned const repetitions = config.value(iterations);
            unsigned const measured = config.value(samples_count);
            unsigned const warmup = config.value(warmups);
            if (size == 0u || repetitions == 0u || measured == 0u)
                throw std::invalid_argument("lengths, iterations and samples must be positive");
            if (static_cast<std::uint64_t>(measured) + warmup >
                std::numeric_limits<unsigned>::max())
                throw std::overflow_error("too many rounds");
            // Prepare the input before starting the timer.
            std::vector<std::uint64_t> data(size);
            std::uint64_t expected = 0u;
            for (std::size_t i = 0u; i < data.size(); ++i) {
                data[i] = static_cast<std::uint64_t>(i + 1u);
                expected += data[i];
            }
            Bench::Samples results;
            for (unsigned n = 0u; n < measured + warmup; ++n) {
                bool const warming_up = n < warmup;
                log.execution_started("sum", n + 1u, warming_up);
                std::uint64_t sum = 0u;
                double const elapsed = Bench::measure_ns([&] {
                    for (unsigned k = 0u; k < repetitions; ++k) {
                        for (auto const value : data) sum += value;
                        Bench::keep_alive(sum);
                    }
                });
                // Verification and Logging are always outside the timer.
                if (sum != expected * repetitions)
                    throw std::runtime_error("incorrect sum");
                if (not warming_up) {
                    double const normalized = elapsed / repetitions;
                    results.add(normalized);
                    log.measurement("sum", n + 1u, elapsed, normalized);
                }
                log.execution_completed("sum", n + 1u, warming_up);
            }
            report.add(config.label(), "sum", Bench::summarize(results));
            log.configuration_completed(config);
        });
        report.print();
        return 0;
    } catch (std::exception const& error) {
        LOGGING_PRINTLN("tutorial_benchmarking: " << error.what())
        return 1;
    }
}

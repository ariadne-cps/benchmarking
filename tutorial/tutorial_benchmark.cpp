#include "ariadne-benchmarking.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace Ariadne;
namespace Bench = Ariadne::Benchmarking;

int main(int argc, char const* argv[]) {
    try {
        unsigned iterations = 1000u;
        unsigned samples_count = 7u;
        unsigned warmups = 2u;

        Bench::CommandLine cli;
        List<ArgumentParser> options;
        auto number = [&](String const& id, unsigned& target) {
            options.append(make_valued_argument_parser(
                "", id, "Positive number",
                [&target](String const& input) -> VoidFunction {
                    unsigned const value = Bench::parse_unsigned<unsigned>(input);
                    if (value == 0u) throw std::invalid_argument("must be positive");
                    return [&target, value] { target = value; };
                }));
        };
        number("iterations", iterations);
        number("samples", samples_count);
        number("warmups", warmups);
        cli.add(CommandLineContribution("tutorial-benchmark", options));
        if (not cli.acquire(Bench::arguments(argc, argv))) return 0;

        // 1. Construct the deterministic input outside timed loops.
        std::vector<std::uint64_t> data(256u);
        for (std::size_t i = 0; i < data.size(); ++i)
            data[i] = static_cast<std::uint64_t>(i + 1u);

        Bench::Samples timings;
        for (unsigned sample = 0u; sample < warmups + samples_count; ++sample) {
            // 2. Measure only the operation; keep the result observable.
            std::uint64_t result = 0u;
            double elapsed = Bench::measure_ns([&] {
                for (unsigned k = 0; k < iterations; ++k) {
                    for (auto const value : data) result += value;
                    Bench::keep_alive(result);
                }
            });
            // 3. Validate AFTER the timed region.
            if (result != 32896u * static_cast<std::uint64_t>(iterations))
                throw std::runtime_error("incorrect sum");
            // 4. Discard the warm-up rounds.
            if (sample >= warmups)
                timings.add(elapsed / static_cast<double>(iterations));
        }
        // 5. Summarize and report; never log inside the timed loop.
        Bench::Report report("Ariadne Benchmarking tutorial");
        report.add("256 integers", "sum", Bench::summarize(timings));
        report.print();
        return 0;
    } catch (std::exception const& error) {
        LOGGING_PRINTLN("tutorial_benchmark: " << error.what())
        return 1;
    }
}

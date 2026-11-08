#include "ariadne-benchmarking.hpp"

#include <stdexcept>

using namespace Ariadne;
namespace Bench = Ariadne::Benchmarking;

// Two semantically equivalent implementations of the same operation.
struct VariantA {
    static unsigned increment(unsigned value) { return value + 1u; }
};
struct VariantB {
    static unsigned increment(unsigned value) { return 1u + value; }
};

int main(int argc, char const* argv[]) {
    Bench::CommandLine cli;
    if (not cli.acquire(Bench::arguments(argc, argv))) return 0;

    Bench::Matrix matrix;  // One fixed configuration.
    Bench::Runner runner(cli);
    Bench::ComparisonBattery<VariantA, VariantB> battery({"A", "B"});

    // The workload is compiled separately for each concrete implementation.
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
    battery.run(matrix, runner, 5u, workload);

    // A single paired report makes the A/B comparison clear.
    Bench::ComparisonReports reports;
    reports.add({"A/B", Bench::ComparisonReportKind::Pairs,
                 {"operation", "reference_ns", "candidate_ns", "slowdown"}});
    reports.print_all(battery.results(), "A");
    return 0;
}

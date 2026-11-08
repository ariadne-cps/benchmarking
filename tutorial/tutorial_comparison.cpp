#include "ariadne-benchmarking.hpp"

#include <stdexcept>

using namespace Ariadne;
namespace Bench = Ariadne::Benchmarking;

// Synthetic equivalent functions. The tutorial demonstrates the API, not
// measured performance differences between real libraries.
struct VariantA {
    static unsigned increment(unsigned value) { return value + 1u; }
};
struct VariantB {
    static unsigned increment(unsigned value) { return 1u + value; }
};
struct VariantC {
    static unsigned increment(unsigned value) { return value - (~0u); }
};

int main(int argc, char const* argv[]) {
    Bench::Parameter<unsigned> lengths("lengths","Workload lengths",{16u,64u});
    Bench::Matrix matrix;
    matrix.add(lengths);
    Bench::CommandLine cli;
    cli.add_parameters(matrix);
    if (not cli.acquire(Bench::arguments(argc,argv))) return 0;

    Bench::Runner runner(cli,matrix.combinations());
    Bench::ComparisonBattery<VariantA,VariantB,VariantC>
        battery({"A","B","C"});

    // The workload is instantiated once per concrete implementation type
    // and receives each current configuration (no virtual timed operation).
    auto workload = [&]<class Implementation>(
            Bench::Configuration const& config, auto&& record) {
        unsigned const length = config.value(lengths);
        unsigned value = 0u;
        record("increment",
            [&] { value = 0u; },
            [&] {
                for (unsigned i = 0u; i < length; ++i) {
                    value = Implementation::increment(value);
                    Bench::keep_alive(value);
                }
            },
            [&] {
                if (value != length)
                    throw std::runtime_error("incorrect increment result");
            }, static_cast<double>(length));
        record("increment_twice",
            [&] { value = 0u; },
            [&] {
                for (unsigned i = 0u; i < length; ++i) {
                    value = Implementation::increment(
                        Implementation::increment(value));
                    Bench::keep_alive(value);
                }
            },
            [&] {
                if (value != 2u * length)
                    throw std::runtime_error("incorrect doubled increment");
            }, static_cast<double>(length));
    };
    battery.run(matrix,runner,3u,workload);

    Bench::ComparisonReports reports;
    reports.add({"timings",Bench::ComparisonReportKind::Measurements,
                 {"implementation","operation","param:lengths","min","mean","mad"}});
    reports.add({"pairs",Bench::ComparisonReportKind::Pairs,
                 {"candidate","operation","param:lengths",
                  "reference_ns","candidate_ns","slowdown"}});
    Bench::ComparisonReportDefinition total{"overall",
        Bench::ComparisonReportKind::Aggregates,
        {"candidate","operations","geomean","total_time","worst"}};
    total.by_configuration = false;
    reports.add(total);
    reports.print_all(battery.results(),"A");
    return 0;
}

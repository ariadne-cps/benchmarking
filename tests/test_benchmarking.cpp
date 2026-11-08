#include "ariadne-benchmarking.hpp"
#include "ariadne-testing.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

using namespace Ariadne::Benchmarking;
using namespace Ariadne::Testing;

int main() {
    Suite suite("Benchmarking core");
    suite.add("statistics", [](Context& ctx) {
        Samples samples;
        for (double n : {1.0, 2.0, 3.0, 4.0}) samples.add(n);
        auto const result = summarize(samples);
        ctx.prove(result.count == 4 && result.median == 2.5
                  && result.mean == 2.5 && result.minimum == 1.0
                  && result.maximum == 4.0 && result.p95 == 4.0
                  && result.mad == 1.0, "summary of even samples");
    });
    suite.add("calibration and ratios", [](Context& ctx) {
        ctx.prove(calibrated_repetitions(10.0, 100.0, 1u, 100u) == 10u,
                  "calibrated repetitions");
        ctx.prove(calibrated_repetitions(10.0, 1000.0, 1u, 15u) == 15u,
                  "bounded calibration");
        ctx.prove(speedup(8.0, 4.0) == 2.0,
                  "speedup convention");
        ctx.prove(std::abs(geometric_mean({2.0, 8.0}) - 4.0) < 1e-12,
                  "geometric mean");
    });
    suite.add("invalid input", [](Context& ctx) {
        bool rejected = false;
        try { (void)summarize(Samples{}); }
        catch (std::invalid_argument const&) { rejected = true; }
        ctx.prove(rejected, "empty samples rejected");
        rejected = false;
        try { (void)parse_unsigned<unsigned>(Ariadne::String("-1")); }
        catch (std::invalid_argument const&) { rejected = true; }
        ctx.prove(rejected, "negative unsigned rejected");
        rejected = false;
        try { Samples samples; samples.add(-1.0); }
        catch (std::invalid_argument const&) { rejected = true; }
        ctx.prove(rejected, "negative timings rejected");
    });
    suite.add("threading command line", [](Context& ctx) {
        CommandLine cli;
        Ariadne::List<Ariadne::String> args;
        args.append(Ariadne::String("test_benchmarking"));
        args.append(Ariadne::String("--concurrency"));
        args.append(Ariadne::String("0"));
        bool const acquired = cli.acquire(args);
        ctx.prove(acquired and Ariadne::ThreadManager::instance().concurrency() == 0u,
                  "Threading CLI applies concurrency");
    });
    suite.add("timing boundaries", [](Context& ctx) {
        int invocations = 0;
        double const elapsed = measure_ns([&] { ++invocations; });
        ctx.prove(invocations == 1 && elapsed >= 0.0,
                  "exactly one invocation");
        Report report("Unit test");
        Samples samples;
        samples.add(12.0);
        report.add("case", "operation", summarize(samples));
        ctx.prove(true, "report can be constructed");
    });
    ConsoleReporter reporter(std::cout);
    auto const outcome = suite.run(reporter);
    return outcome.successful() ? 0 : 1;
}

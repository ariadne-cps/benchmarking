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
                  && result.mad == 1.0
                  && std::abs(result.variance - 5.0/3.0) < 1e-12
                  && std::abs(result.stddev - std::sqrt(5.0/3.0)) < 1e-12,
                  "summary of even samples with sample dispersion");
    });
    suite.add("summary columns and CLI selection", [](Context& ctx) {
        ctx.prove(default_summary_fields() ==
                  SummaryFields({SummaryField::Minimum,SummaryField::Maximum,
                                 SummaryField::Mean,SummaryField::MAD}),
                  "default summary columns and order");
        ctx.prove(parse_summary_fields(" stddev, min , count ") ==
                  SummaryFields({SummaryField::Stddev,SummaryField::Minimum,
                                 SummaryField::Count}),
                  "custom order preserved");
        for (std::string const& invalid : {"", "min,", ",max", "min,,max",
                                           "median,median", "unknown"}) {
            bool rejected = false;
            try { (void)parse_summary_fields(invalid); }
            catch (std::invalid_argument const&) { rejected = true; }
            ctx.prove(rejected, "invalid or duplicate column rejected");
        }
        CommandLine cli;
        Ariadne::List<Ariadne::String> args;
        args.append(Ariadne::String("test_benchmarking"));
        args.append(Ariadne::String("--summary"));
        args.append(Ariadne::String("max,mean,variance,stddev"));
        bool const accepted = cli.acquire(args);
        ctx.prove(accepted && cli.summary_fields() ==
                  SummaryFields({SummaryField::Maximum,SummaryField::Mean,
                                 SummaryField::Variance,SummaryField::Stddev}),
                  "CLI applies a custom summary column selection");
    });
    suite.add("report output and single sample dispersion", [](Context& ctx) {
        Samples samples;
        samples.add(10.0);
        auto const stats = summarize(samples);
        ctx.prove(std::isnan(stats.variance) && std::isnan(stats.stddev),
                  "single sample dispersion is undefined");
        Report defaults("Default");
        defaults.add("one", "operation", stats);
        std::string const header = defaults.table().substr(
            0u, defaults.table().find('\n'));
        auto const min_pos = header.find("Min (ns)");
        auto const max_pos = header.find("Max (ns)");
        auto const mean_pos = header.find("Mean (ns)");
        auto const mad_pos = header.find("MAD (ns)");
        ctx.prove(min_pos != std::string::npos && min_pos < max_pos &&
                  max_pos < mean_pos && mean_pos < mad_pos &&
                  header.find("Median (ns)") == std::string::npos,
                  "default table only prints min max mean MAD in order");
        Report custom("Custom", parse_summary_fields(
            "count,stddev,variance,median,p95"));
        custom.add("one", "operation", stats);
        std::string const table = custom.table();
        ctx.prove(table.find("Variance (ns^2)") != std::string::npos &&
                  table.find("Stddev (ns)") != std::string::npos &&
                  table.find("n/a") != std::string::npos &&
                  table.find("Median (ns)") != std::string::npos,
                  "custom report labels and undefined dispersion rendered");
    });
    suite.add("stable sample dispersion", [](Context& ctx) {
        Samples samples;
        samples.add(1000000001.0);
        samples.add(1000000002.0);
        samples.add(1000000003.0);
        auto const stats = summarize(samples);
        ctx.prove(stats.mean == 1000000002.0 &&
                  std::abs(stats.variance - 1.0) < 1e-12 &&
                  std::abs(stats.stddev - 1.0) < 1e-12,
                  "sample variance stable for large offsets");
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
    suite.add("benchmark parameter matrix", [](Context& ctx) {
        Parameter<unsigned> size("size", "Size", {2u, 4u});
        Parameter<std::string> backend("backend", "Backend", {"fast", "slow"});
        Matrix matrix;
        matrix.add(size).add(backend);
        std::vector<std::string> seen;
        matrix.for_each([&](Configuration const& config) {
            seen.push_back(config.label());
            ctx.prove(config.value(size) == 2u || config.value(size) == 4u,
                      "typed parameter lookup");
        });
        ctx.prove(matrix.combinations() == 4u &&
                  seen == std::vector<std::string>({
                      "size=2, backend=fast", "size=2, backend=slow",
                      "size=4, backend=fast", "size=4, backend=slow"}),
                  "Cartesian product order");
        ctx.prove(detail::quote_if_needed("backend") == "backend" &&
                  detail::quote_if_needed("fast") == "fast" &&
                  detail::quote_if_needed("hello world") == std::string(1, '"') + "hello world" + '"',
                  "quote only separators and whitespace");
        ctx.prove(matrix.header() ==
                  "Parameters: size=[2, 4]; backend=[fast, slow] | configurations=4",
                  "all parameter sequences listed in header");
    });
    suite.add("list CLI parsing and duplicates", [](Context& ctx) {
        Parameter<unsigned> size("size", "Size", {7u});
        Matrix matrix;
        matrix.add(size);
        CommandLine cli;
        cli.add_parameters(matrix);
        Ariadne::List<Ariadne::String> args;
        args.append(Ariadne::String("test_benchmarking"));
        args.append(Ariadne::String("--size"));
        args.append(Ariadne::String("2, 3,2"));
        bool const accepted = cli.acquire(args);
        ctx.prove(accepted &&
                  size.values() == std::vector<unsigned>({2u,3u,2u}),
                  "CLI retains order and duplicate values");
        bool invalid = false;
        try { (void)detail::split_values("1,,3"); }
        catch (std::invalid_argument const&) { invalid = true; }
        ctx.prove(invalid, "empty list members rejected");
        invalid = false;
        try { (void)detail::split_values("\"unterminated"); }
        catch (std::invalid_argument const&) { invalid = true; }
        ctx.prove(invalid, "unterminated quoted value rejected");
        ctx.prove(detail::split_values("a,\"b,c\",d") ==
                  std::vector<std::string>({"a","b,c","d"}),
                  "quoted comma preserved in a string value");
    });
    suite.add("verbosity depth", [](Context& ctx) {
        RunLog single(1u);
        RunLog matrix(2u);
        ctx.prove(single.execution_level() == 2u &&
                  single.measurement_level() == 3u &&
                  matrix.execution_level() == 3u &&
                  matrix.measurement_level() == 4u,
                  "matrix adds exactly one detail level");
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

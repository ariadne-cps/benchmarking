#include "ariadne-benchmarking.hpp"
#include "ariadne-testing.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <limits>

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
        for (char const* invalid : {"", "min,", ",max", "min,,max",
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
    suite.add("table widths use actual displayed values", [](Context& ctx) {
        Table table({
            {"Backend"}, {"Operation"},
            {"Bits", 0u, TableAlignment::Right},
            {"Median (ns)", 0u, TableAlignment::Right}
        }, TableOptions::ruled());
        table.add_row({"floatdp", "add", "53", Table::number(2.5)});
        table.add_row({"floatmp", "a_very_long_operation_name", "1024",
                       Table::number(1234567.125)});
        auto const result = table.str();
        auto const first_end = result.find('\n');
        auto const header_end = result.find('\n', first_end + 1u);
        auto const separator_end = result.find('\n', header_end + 1u);
        auto const first_row_end = result.find('\n', separator_end + 1u);
        auto const second_row_end = result.find('\n', first_row_end + 1u);
        auto const header = result.substr(first_end + 1u, header_end-first_end-1u);
        auto const short_row = result.substr(separator_end + 1u, first_row_end-separator_end-1u);
        auto const long_row = result.substr(first_row_end + 1u, second_row_end-first_row_end-1u);
        ctx.prove(header.find("Operation") != std::string::npos &&
                  header.find("Median (ns)") != std::string::npos,
                  "heading contains dimensions and measures");
        ctx.prove(short_row.size() == long_row.size() &&
                  short_row.size() == header.size(),
                  "columns use longest rendered cell, including long operation");
        ctx.prove(long_row.find("1234567.125") != std::string::npos &&
                  short_row.find("2.500") != std::string::npos,
                  "numeric columns preserve formatted content");
        ctx.prove(result.substr(0u, first_end) ==
                  std::string(header.size(), '='),
                  "outer rule spans computed table width");
    });
    suite.add("group separators and optional minimum widths", [](Context& ctx) {
        auto options = TableOptions::ruled();
        options.group_columns = {0u};
        Table grouped({{"Backend"}, {"Operation"}, {"Bits"}}, options);
        grouped.add_row({"floatdp", "add", "53"});
        grouped.add_row({"floatdp", "sub", "53"});
        grouped.add_row({"floatmp", "add", "53"});
        auto const output = grouped.str();
        auto const row_separator = output.find("\n-", output.find("sub"));
        ctx.prove(row_separator != std::string::npos &&
                  row_separator < output.find("floatmp", row_separator),
                  "group separator inserted when backend changes");
        Table padded({{"A", 10u}, {"B"}});
        padded.add_row({"x", "value"});
        ctx.prove(padded.str().find("x         ") != std::string::npos,
                  "explicit minimum width supported but not imposed by default");
    });
    suite.add("custom report dimensions and computed numeric widths",
              [](Context& ctx) {
        Samples a;
        a.add(2.0);
        Samples b;
        b.add(123456789.0);
        Report report("Comparison", {SummaryField::Median});
        report.set_dimension_columns({
            {"Backend"}, {"Operation"}, {"Bits", 0u, TableAlignment::Right}
        });
        auto options = TableOptions::ruled();
        options.group_columns = {0u};
        report.set_table_options(options);
        report.add_row({"floatdp", "add", "53"}, summarize(a));
        report.add_row({"floatmp", "long_operation_name", "53"}, summarize(b));
        auto const output = report.table();
        ctx.prove(output.find("Backend") != std::string::npos &&
                  output.find("Operation") != std::string::npos &&
                  output.find("Bits") != std::string::npos &&
                  output.find("Median (ns)") != std::string::npos,
                  "descriptive and statistical columns share a generic table");
        ctx.prove(output.find("123456789.000") != std::string::npos &&
                  output.find("long_operation_name") != std::string::npos,
                  "long numeric and descriptive values are fully displayed");
    });
    suite.add("templated benchmark battery", [](Context& ctx) {
        Parameter<unsigned> sizes("bits","Size",{8u,16u});
        Matrix matrix;
        matrix.add(sizes);
        Runner runner(1u,matrix.combinations());
        ComparisonBattery<int,long,double> battery({"A","B","C"});
        unsigned verifications = 0u;
        auto workload = [&]<class Implementation>(auto&& record) {
            unsigned value = 0u;
            unsigned const factor = sizeof(Implementation) > sizeof(int) ? 2u : 1u;
            record("increment",
                [&] { value = 0u; },
                [&] { value += factor; keep_alive(value); },
                [&] {
                    if (value != factor) throw std::runtime_error("incorrect increment");
                    ++verifications;
                });
            record("double",
                [&] { value = 0u; },
                [&] { value = 2u * factor; keep_alive(value); },
                [&] {
                    if (value != 2u * factor) throw std::runtime_error("incorrect double");
                    ++verifications;
                });
        };
        battery.run(matrix,runner,2u,workload);
        ctx.prove(battery.results().measurements().size() == 12u &&
                  verifications == 36u,
                  "three implementations times two operations times two configurations");
        ctx.prove(battery.results().paired("A").size() == 8u,
                  "two candidates and complete operation pairing");
    });
    suite.add("deterministic N-way comparisons and summaries", [](Context& ctx) {
        ComparisonResults results;
        for (auto const& name : {"A","B","C"})
            results.register_implementation(name);
        ComparisonContext first;
        first.label = "bits=53";
        first.dimensions["bits"] = "53";
        ComparisonContext second;
        second.ordinal = 2u;
        second.label = "bits=128";
        second.dimensions["bits"] = "128";
        auto add = [&](std::string name,std::string operation,
                       ComparisonContext config,double ns) {
            Samples samples;
            samples.add(ns);
            samples.add(ns);
            results.add(std::move(name),std::move(operation),config,samples);
        };
        for (auto const& config : {first,second}) {
            add("A","add",config,10.0);
            add("A","mul",config,20.0);
            add("B","add",config,20.0);
            add("B","mul",config,40.0);
            add("C","add",config,5.0);
            add("C","mul",config,10.0);
        }
        auto const paired = results.paired("A");
        ctx.prove(paired.size() == 8u &&
                  std::abs(paired.front().slowdown-2.0) < 1e-12 &&
                  std::abs(paired.front().speedup-0.5) < 1e-12,
                  "slowdown is candidate/reference and speedup its reciprocal");
        auto const totals = results.aggregates("A",false);
        ctx.prove(totals.size() == 2u && totals.front().operations == 4u &&
                  std::abs(totals.front().geomean-2.0) < 1e-12 &&
                  std::abs(totals.front().total_time-2.0) < 1e-12 &&
                  std::abs(totals.front().worst-2.0) < 1e-12 &&
                  std::abs(totals.back().geomean-0.5) < 1e-12,
                  "per-candidate geomean, total time, worst and direction");
        ctx.prove(results.aggregates("A",true).size() == 4u,
                  "per-configuration aggregates");
        bool duplicate = false;
        try { add("B","add",first,20.0); }
        catch (std::invalid_argument const&) { duplicate = true; }
        ctx.prove(duplicate,"duplicate measurement rejected");
        bool invalid = false;
        try { (void)results.paired("A", SummaryField::MAD); }
        catch (std::invalid_argument const&) { invalid = true; }
        ctx.prove(invalid,"dispersion not accepted as comparison timing");
    });
    suite.add("strict pairing and configuration metadata", [](Context& ctx) {
        ComparisonResults results;
        results.register_implementation("A");
        results.register_implementation("B");
        Samples samples;
        samples.add(10.0);
        results.add("A","add",ComparisonContext{},samples);
        bool missing = false;
        try { (void)results.paired("A"); }
        catch (std::invalid_argument const&) { missing = true; }
        ctx.prove(missing,"missing candidate operations rejected");
        results.add("B","add",ComparisonContext{},samples);
        ctx.prove(results.paired("A").size() == 1u,"exact pairing accepted");
        ComparisonContext conflicting;
        conflicting.label = "different";
        bool inconsistent = false;
        try { results.add("B","mul",conflicting,samples); }
        catch (std::invalid_argument const&) { inconsistent = true; }
        ctx.prove(inconsistent,"conflicting configuration metadata rejected");
    });
    suite.add("independently configured comparison reports", [](Context& ctx) {
        ComparisonResults data;
        data.register_implementation("floatdp");
        data.register_implementation("floatmp");
        ComparisonContext config;
        config.label = "bits=53";
        config.dimensions["bits"] = "53";
        Samples baseline,candidate;
        baseline.add(10.0);
        baseline.add(10.0);
        candidate.add(20.0);
        candidate.add(20.0);
        data.add("floatdp","add",config,baseline);
        data.add("floatmp","add",config,candidate);
        ComparisonReports reports;
        reports.add({"raw",ComparisonReportKind::Measurements,
                     {"implementation","operation","param:bits","min","mad"}});
        reports.add({"paired",ComparisonReportKind::Pairs,
                     {"operation","param:bits","candidate_ns",
                      "reference_ns","slowdown"}});
        ComparisonReportDefinition overall{"total",ComparisonReportKind::Aggregates,
                     {"candidate","operations","geomean","total_time","worst"}};
        overall.by_configuration = false;
        reports.add(overall);
        auto const raw = reports.render("raw",data,"floatdp");
        auto const paired = reports.render("paired",data,"floatdp");
        auto const total = reports.render("total",data,"floatdp");
        ctx.prove(raw.find("Implementation") != std::string::npos &&
                  raw.find("MAD (ns)") != std::string::npos &&
                  raw.find("Slowdown") == std::string::npos,
                  "raw measurement view has independent column selection");
        ctx.prove(paired.find("Slowdown (x)") != std::string::npos &&
                  paired.find("2.000") != std::string::npos &&
                  paired.find("Min (ns)") == std::string::npos,
                  "paired comparison view has independent column selection");
        ctx.prove(total.find("Total time (x)") != std::string::npos &&
                  total.find("Worst (x)") != std::string::npos &&
                  total.find("2.000") != std::string::npos,
                  "aggregate view has independent columns");
        reports.add({"invalid",ComparisonReportKind::Pairs,{"unrecognized"}});
        bool rejected = false;
        try { (void)reports.render("invalid",data,"floatdp"); }
        catch (std::invalid_argument const&) { rejected = true; }
        ctx.prove(rejected,"invalid columns rejected");
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
    suite.add("global warmup CLI", [](Context& ctx) {
        CommandLine defaults;
        ctx.prove(defaults.warmups() == 0u,
                  "default warmup count is zero");
        CommandLine cli;
        Ariadne::List<Ariadne::String> args;
        args.append(Ariadne::String("test_benchmarking"));
        args.append(Ariadne::String("--warmups"));
        args.append(Ariadne::String("2"));
        bool const accepted = cli.acquire(args);
        ctx.prove(accepted && cli.warmups() == 2u,
                  "global warmup CLI accepted");
        Runner configured(cli);
        ctx.prove(configured.warmups() == 2u,
                  "runner inherits global warmup count");
    });
    suite.add("warmups are verified and excluded from statistics",
              [](Context& ctx) {
        unsigned prepared = 0u;
        unsigned executed = 0u;
        unsigned verified = 0u;
        unsigned value = 0u;
        Runner runner(2u);
        Samples const samples = runner.run(
            "work", 3u,
            [&] { ++prepared; value = 0u; },
            [&] { ++executed; value = 7u; },
            [&] {
                ++verified;
                if (value != 7u)
                    throw std::runtime_error("invalid benchmark result");
            });
        ctx.prove(prepared == 5u && executed == 5u && verified == 5u,
                  "warmups run full prepare, measure and verify phases");
        ctx.prove(samples.size() == 3u,
                  "only measured executions are sampled");
    });
    suite.add("default runner performs no warmups", [](Context& ctx) {
        Runner runner;
        unsigned executed = 0u;
        Samples const samples = runner.run(
            "work", 2u,
            [] {},
            [&] { ++executed; },
            [] {});
        ctx.prove(runner.warmups() == 0u && executed == 2u &&
                  samples.size() == 2u,
                  "zero warmup default preserves sample count");
    });
    suite.add("warmup verification failures propagate", [](Context& ctx) {
        Runner runner(1u);
        unsigned executed = 0u;
        bool rejected = false;
        try {
            (void)runner.run("invalid", 1u,
                [] {},
                [&] { ++executed; },
                [&] { throw std::runtime_error("verification failed"); });
        } catch (std::runtime_error const&) { rejected = true; }
        ctx.prove(rejected && executed == 1u,
                  "invalid warmup aborts rather than silently discarding");
    });
    suite.add("runner rejects invalid counts and normalization",
              [](Context& ctx) {
        Runner runner(1u);
        bool rejected = false;
        try {
            (void)runner.run("zero", 0u, [] {}, [] {}, [] {});
        } catch (std::invalid_argument const&) { rejected = true; }
        ctx.prove(rejected, "zero measured samples rejected");
        rejected = false;
        try {
            (void)runner.run("invalid norm", 1u, [] {}, [] {}, [] {}, 0.0);
        } catch (std::invalid_argument const&) { rejected = true; }
        ctx.prove(rejected, "invalid normalization rejected");
        rejected = false;
        try {
            (void)runner.run("overflow",
                std::numeric_limits<unsigned>::max(),
                [] {}, [] {}, [] {});
        } catch (std::overflow_error const&) { rejected = true; }
        ctx.prove(rejected, "round overflow rejected before execution");
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

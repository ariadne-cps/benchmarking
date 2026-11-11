/***************************************************************************
 *            test_comparison.cpp
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
#include "ariadne-testing.hpp"
#include "log_capture.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <limits>

using namespace Ariadne::Benchmarking;
using namespace Ariadne::Testing;

class TestComparison : public ContextFixture {
public:
    using ContextFixture::ContextFixture;

    void test_aggregate_ratio_rounding_overflow() {
        auto aggregate = [](std::vector<std::pair<double, double>> const& times) {
            ComparisonResults data;
            data.register_implementation("A");
            data.register_implementation("B");
            std::size_t index = 0u;
            for (auto const& [reference, candidate] : times) {
                auto const operation = std::to_string(++index);
                Statistics stats{};
                stats.median = reference;
                data.add("A", operation, {}, stats);
                stats.median = candidate;
                data.add("B", operation, {}, stats);
            }
            return data.aggregates("A");
        };
        // All individual ratios are finite. Accumulated logarithm rounding
        // near log(DBL_MAX) can nevertheless overflow the geometric mean.
        double const reference = std::numeric_limits<double>::min();
        double const candidate = std::nextafter(4.0, 0.0);
        ARIADNE_TESTING_PROVE(std::isfinite(candidate / reference));
        ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::overflow_error,
            aggregate(std::vector<std::pair<double, double>>(70u, {reference, candidate})),
            "ratio overflow");
        // These binary boundary values have finite individual ratios and
        // finite sums, but rounding the sums makes their quotient overflow.
        std::vector<std::pair<double, double>> const rounded{
            {0x1.ee0caea7935a2p-1015, 0x1.ee0caea7935a1p+9},
            {0x1.2999b73f3d146p-1019, 0x1.2999b73f3d145p+5}};
        for (auto const& [ref, cand] : rounded)
            ARIADNE_TESTING_PROVE(std::isfinite(cand / ref));
        ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::overflow_error,
                                            aggregate(rounded), "ratio overflow");
    }

    void test_report_definition_validation() {
        ComparisonReports reports;
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            reports.add({"", ComparisonReportKind::Measurements, {"operation"}}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            reports.add({"empty", ComparisonReportKind::Measurements, {}}));
        reports.add({"raw", ComparisonReportKind::Measurements, {"operation"}});
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            reports.add({"raw", ComparisonReportKind::Measurements, {"operation"}}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            reports.add({"empty column", ComparisonReportKind::Measurements, {""}}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            reports.add({"duplicate", ComparisonReportKind::Measurements, {"operation", "operation"}}));
        ComparisonResults empty;
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, reports.render("unknown", empty, "A"));
        reports.add({"dimension", ComparisonReportKind::Measurements, {"param:size"}});
        ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::invalid_argument,
            reports.render("dimension", empty, "A"), "no measurements");
        Statistics stats{};
        empty.register_implementation("A");
        empty.add("A", "op", {}, stats);
        ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::invalid_argument,
            reports.render("dimension", empty, "A"), "missing report parameter");
        reports.add({"unsupported", ComparisonReportKind::Measurements, {"speedup"}});
        ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::invalid_argument,
            reports.render("unsupported", empty, "A"), "unsupported report column");
    }

    void test_complete_comparison_views() {
        ComparisonResults data;
        data.register_implementation("A");
        data.register_implementation("B");
        ComparisonContext config;
        config.label = "size=2";
        config.dimensions["size"] = "2";
        Samples samples;
        samples.add(2.0);
        samples.add(4.0);
        data.add("A", "add", config, samples);
        data.add("B", "add", config, samples);
        auto standard = ComparisonReports::standard();
        auto const all = standard.render_all(data, "A");
        ARIADNE_TESTING_PROVE(all.starts_with("measurements\n"));
        ARIADNE_TESTING_PROVE(all.find("\n\npairs\n") != std::string::npos);
        ARIADNE_TESTING_PROVE(all.find("\n\naggregates\n") != std::string::npos);
        ARIADNE_TESTING_PROVE(ComparisonReports{}.render_all(data, "A").empty());
        ComparisonReports full;
        full.add({"raw", ComparisonReportKind::Measurements,
            {"implementation", "operation", "configuration", "param:size",
             "min", "max", "mean", "mad", "median", "p95", "stddev", "variance", "count"}});
        full.add({"pairs", ComparisonReportKind::Pairs,
            {"reference", "candidate", "operation", "configuration", "param:size",
             "reference_ns", "candidate_ns", "slowdown", "speedup"}});
        full.add({"aggregate", ComparisonReportKind::Aggregates,
            {"reference", "candidate", "configuration", "operations", "geomean",
             "total_time", "worst", "worst_operation", "worst_configuration"}});
        auto const raw = full.render("raw", data, "A");
        ARIADNE_TESTING_PROVE(raw.find("Variance (ns^2)") != std::string::npos);
        ARIADNE_TESTING_PROVE(raw.find("4.000") != std::string::npos);
        auto const pairs = full.render("pairs", data, "A");
        ARIADNE_TESTING_PROVE(pairs.find("Speedup (x)") != std::string::npos);
        ARIADNE_TESTING_PROVE(pairs.find("1.000") != std::string::npos);
        auto const aggregate = full.render("aggregate", data, "A");
        ARIADNE_TESTING_PROVE(aggregate.find("Worst configuration") != std::string::npos);
        ARIADNE_TESTING_PROVE(aggregate.find("size=2") != std::string::npos);
        full.add({"invalid aggregate dimension", ComparisonReportKind::Aggregates, {"param:size"}});
        ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::invalid_argument,
            full.render("invalid aggregate dimension", data, "A"), "cannot select a parameter");
        LogCapture capture;
        capture.verbosity(0u);
        standard.print_all(data, "A");
        ARIADNE_TESTING_PROVE(capture.str().empty());
        capture.verbosity(1u);
        standard.print_all(data, "A");
        ARIADNE_TESTING_PROVE(capture.str().find("measurements") != std::string::npos);
        ARIADNE_TESTING_PROVE(capture.str().find("aggregates") != std::string::npos);
        // Regress a misleading test configuration that used to allow
        // print_all() to emit at current_level + 1. The report belongs
        // to the current level and must be visible without that extra step.
        capture.clear();
        auto& logger = Ariadne::Logger::instance();
        logger.configuration().set_verbosity(logger.current_level());
        standard.print_all(data, "A");
        ARIADNE_TESTING_PROVE(capture.str().find("aggregates") != std::string::npos);
    }

    void test_registration_and_keys() {
        ComparisonResults results;
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, results.register_implementation(""));
        results.register_implementation("A");
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, results.register_implementation("A"));
        ARIADNE_TESTING_EXPECT_EQUAL(results.implementations(), std::vector<std::string>({"A"}));
        Statistics stats{};
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, results.add("B", "op", {}, stats));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, results.add("A", "", {}, stats));
        ComparisonContext invalid;
        invalid.ordinal = 0u;
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, results.add("A", "op", invalid, stats));
        results.add("A", "op", {}, stats);
        ComparisonContext conflicting;
        conflicting.dimensions["size"] = "2";
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                    results.add("A", "other", conflicting, stats));
        ARIADNE_TESTING_EXPECT_EQUAL(results.measurements().size(), 1u);
    }

    void test_pairing_errors_and_timing_metrics() {
        ComparisonResults results;
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, results.paired("missing"));
        results.register_implementation("A");
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, results.paired("A"));
        results.register_implementation("B");
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, results.paired("A"));
        Statistics const stats{3.0, 4.0, 1.0, 9.0, 2.0, 8.0, 4u, 5.0, 6.0};
        results.add("A", "one", {}, stats);
        results.add("B", "other", {}, stats);
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, results.paired("A"));
        ARIADNE_TESTING_EXPECT_EQUAL(ComparisonResults::timing(stats, SummaryField::Minimum), 1.0);
        ARIADNE_TESTING_EXPECT_EQUAL(ComparisonResults::timing(stats, SummaryField::Maximum), 9.0);
        ARIADNE_TESTING_EXPECT_EQUAL(ComparisonResults::timing(stats, SummaryField::Mean), 4.0);
        ARIADNE_TESTING_EXPECT_EQUAL(ComparisonResults::timing(stats, SummaryField::Median), 3.0);
        ARIADNE_TESTING_EXPECT_EQUAL(ComparisonResults::timing(stats, SummaryField::P95), 8.0);
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                    ComparisonResults::timing(stats, SummaryField::Count));
    }

    void test_ratio_validation_and_overflow() {
        auto compare = [](double reference, double candidate) {
            ComparisonResults results;
            results.register_implementation("A");
            results.register_implementation("B");
            Statistics stats{};
            stats.median = reference;
            results.add("A", "op", {}, stats);
            stats.median = candidate;
            results.add("B", "op", {}, stats);
            return results.paired("A");
        };
        for (double invalid : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::quiet_NaN()}) {
            auto scope = ctx().scope("invalid comparison time", invalid);
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, compare(invalid, 1.0));
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, compare(1.0, invalid));
        }
        double const tiny = std::numeric_limits<double>::denorm_min();
        double const huge = std::numeric_limits<double>::max();
        ARIADNE_TESTING_EXPECT_THROWS(std::overflow_error, compare(tiny, huge));
        ARIADNE_TESTING_EXPECT_THROWS(std::overflow_error, compare(huge, tiny));
        auto const pairs = compare(4.0, 2.0);
        ARIADNE_TESTING_REQUIRE(pairs.size() == 1u);
        ARIADNE_TESTING_EXPECT_EQUAL(pairs.front().slowdown, 0.5);
        ARIADNE_TESTING_EXPECT_EQUAL(pairs.front().speedup, 2.0);
    }

    void test_aggregate_sum_overflow() {
        double const huge = std::numeric_limits<double>::max();
        for (auto const& times : std::vector<std::pair<double, double>>{
                {huge, 1.0}, {1.0, huge}}) {
            auto scope = ctx().scope("aggregate times", std::to_string(times.first) +
                                     "/" + std::to_string(times.second));
            ComparisonResults results;
            results.register_implementation("A");
            results.register_implementation("B");
            for (char const* operation : {"one", "two"}) {
                Statistics stats{};
                stats.median = times.first;
                results.add("A", operation, {}, stats);
                stats.median = times.second;
                results.add("B", operation, {}, stats);
            }
            ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::overflow_error,
                                                results.aggregates("A"), "timing overflow");
        }
    }

    void test_battery_configuration_workload_and_empty_registration() {
        ComparisonBattery<int> battery({"A"});
        Runner runner;
        Configuration config({}, {}, 1u, 1u);
        unsigned verified = 0u;
        bool register_operation = false;
        auto workload = [&]<class Implementation>(Configuration const& current, auto&& record) {
            ARIADNE_TESTING_EXPECT_EQUAL(current.ordinal(), 1u);
            if (not register_operation) return;
            Implementation value{};
            record("increment", [&] { value = 0; }, [&] { ++value; }, [&] {
                ARIADNE_TESTING_EXPECT_EQUAL(value, Implementation{1});
                ++verified;
            });
        };
        ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::invalid_argument,
            battery.run(config, runner, 1u, workload), "no registered operations");
        ARIADNE_TESTING_PROVE(battery.results().measurements().empty());
        register_operation = true;
        battery.run(config, runner, 2u, workload);
        ARIADNE_TESTING_EXPECT_EQUAL(verified, 2u);
        ARIADNE_TESTING_REQUIRE(battery.results().measurements().size() == 1u);
        ARIADNE_TESTING_EXPECT_EQUAL(battery.results().measurements().front().statistics.count, 2u);
    }

    void test_templated_benchmark_battery() {
        Parameter<unsigned> sizes("bits","Size",{8u,16u});
        Matrix matrix;
        matrix.add(sizes);
        Runner runner(1u,matrix.combinations());
        ComparisonBattery<int,long,double> battery({"A","B","C"});
        unsigned verifications = 0u;
        bool register_operations = false;
        auto workload = [&]<class Implementation>(auto&& record) {
            if (not register_operations) return;
            Implementation value{};
            record("increment",
                [&] { value = Implementation{}; },
                [&] {
                    for (unsigned i = 0u; i < 1000u; ++i) {
                        value += Implementation{1};
                        keep_alive(value);
                    }
                },
                [&] {
                    if (value != Implementation{1000})
                        throw std::runtime_error("incorrect increment");
                    ++verifications;
                });
            record("double",
                [&] { value = Implementation{}; },
                [&] {
                    for (unsigned i = 0u; i < 1000u; ++i) {
                        value += Implementation{2};
                        keep_alive(value);
                    }
                },
                [&] {
                    if (value != Implementation{2000})
                        throw std::runtime_error("incorrect double");
                    ++verifications;
                });
        };
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                    battery.run(matrix,runner,2u,workload));
        ARIADNE_TESTING_PROVE(battery.results().measurements().empty());
        register_operations = true;
        battery.run(matrix,runner,2u,workload);
        // three implementations times two operations times two configurations
        ARIADNE_TESTING_PROVE(battery.results().measurements().size() == 12u &&
                  verifications == 36u);
        // two candidates and complete operation pairing
        ARIADNE_TESTING_PROVE(battery.results().paired("A").size() == 8u);
    }

    void test_deterministic_n_way_comparisons_and_summaries() {
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
        // slowdown is candidate/reference and speedup its reciprocal
        ARIADNE_TESTING_PROVE(paired.size() == 8u &&
                  std::abs(paired.front().slowdown-2.0) < 1e-12 &&
                  std::abs(paired.front().speedup-0.5) < 1e-12);
        auto const totals = results.aggregates("A",false);
        // per-candidate geomean, total time, worst and direction
        ARIADNE_TESTING_PROVE(totals.size() == 2u && totals.front().operations == 4u &&
                  std::abs(totals.front().geomean-2.0) < 1e-12 &&
                  std::abs(totals.front().total_time-2.0) < 1e-12 &&
                  std::abs(totals.front().worst-2.0) < 1e-12 &&
                  std::abs(totals.back().geomean-0.5) < 1e-12);
        // per-configuration aggregates
        ARIADNE_TESTING_PROVE(results.aggregates("A",true).size() == 4u);
        // duplicate measurement rejected
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            add("B","add",first,20.0));
        // dispersion not accepted as comparison timing
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            results.paired("A", SummaryField::MAD));
    }

    void test_strict_pairing_and_configuration_metadata() {
        ComparisonResults results;
        results.register_implementation("A");
        results.register_implementation("B");
        Samples samples;
        samples.add(10.0);
        results.add("A","add",ComparisonContext{},samples);
        // missing candidate operations rejected
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            results.paired("A"));
        results.add("B","add",ComparisonContext{},samples);
        // exact pairing accepted
        ARIADNE_TESTING_PROVE(results.paired("A").size() == 1u);
        ComparisonContext conflicting;
        conflicting.label = "different";
        // conflicting configuration metadata rejected
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            results.add("B","mul",conflicting,samples));
    }

    void test_independently_configured_comparison_reports() {
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
        // raw measurement view has independent column selection
        ARIADNE_TESTING_PROVE(raw.find("Implementation") != std::string::npos &&
                  raw.find("MAD (ns)") != std::string::npos &&
                  raw.find("Slowdown") == std::string::npos);
        // paired comparison view has independent column selection
        ARIADNE_TESTING_PROVE(paired.find("Slowdown (x)") != std::string::npos &&
                  paired.find("2.000") != std::string::npos &&
                  paired.find("Min (ns)") == std::string::npos);
        // aggregate view has independent columns
        ARIADNE_TESTING_PROVE(total.find("Total time (x)") != std::string::npos &&
                  total.find("Worst (x)") != std::string::npos &&
                  total.find("2.000") != std::string::npos);
        reports.add({"invalid",ComparisonReportKind::Pairs,{"unrecognized"}});
        // invalid columns rejected
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            reports.render("invalid",data,"floatdp"));
    }

};

int main(int argc, char** argv) {
    ARIADNE_TESTING_SUITE("Benchmarking comparison");
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_aggregate_ratio_rounding_overflow);
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_report_definition_validation);
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_complete_comparison_views);
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_registration_and_keys);
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_pairing_errors_and_timing_metrics);
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_ratio_validation_and_overflow);
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_aggregate_sum_overflow);
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_battery_configuration_workload_and_empty_registration);
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_templated_benchmark_battery);
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_deterministic_n_way_comparisons_and_summaries);
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_strict_pairing_and_configuration_metadata);
    ARIADNE_TESTING_ADD_TEST(TestComparison, test_independently_configured_comparison_reports);
    return ARIADNE_TESTING_RUN_TESTS_WITH_ARGS(argc, argv);
}

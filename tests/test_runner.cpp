/***************************************************************************
 *            test_runner.cpp
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

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "log_capture.hpp"

#include "ariadne-benchmarking.hpp"
#include "ariadne-testing.hpp"

using namespace Ariadne::Benchmarking;
using namespace Ariadne::Testing;

class TestRunner : public ContextFixture {
  public:
    using ContextFixture::ContextFixture;

    void test_runner_validation_in_one_instantiation() {
        auto no_op = [] {};
        Runner runner(1u);
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                      runner.run("zero", 0u, no_op, no_op, no_op));
        for (double invalid: {0.0, -1.0, std::numeric_limits<double>::infinity(),
                              std::numeric_limits<double>::quiet_NaN()}) {
            auto scope = ctx().scope("normalization", invalid);
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                          runner.run("normalization", 1u, no_op, no_op, no_op, invalid));
        }
        ARIADNE_TESTING_EXPECT_THROWS(std::overflow_error,
                                      runner.run("overflow", std::numeric_limits<unsigned>::max(), no_op, no_op, no_op));
        ARIADNE_TESTING_EXPECT_EQUAL(runner.run("valid", 2u, no_op, no_op, no_op, 2.0).size(), 2u);
    }

    void test_progress_logging() {
        LogCapture capture;
        Configuration config({}, {}, 1u, 2u);
        for (unsigned verbosity: {0u, 4u}) {
            auto scope = ctx().scope("verbosity", verbosity);
            capture.verbosity(verbosity);
            capture.clear();
            for (std::size_t count: {1u, 2u}) {
                RunLog log(count);
                log.configuration_started(config);
                log.configuration_completed(config);
                log.execution_started("warm", 1u, true);
                log.execution_completed("warm", 1u, true);
                log.execution_started("measured", 2u, false);
                log.measurement("measured", 2u, 12.0, 6.0);
                log.execution_completed("measured", 2u, false);
            }
            const auto output = capture.str();
            if (verbosity == 0u) {
                ARIADNE_TESTING_PROVE(output.empty());
            } else {
                ARIADNE_TESTING_PROVE(output.find("Configuration 1/2 started") != std::string::npos);
                ARIADNE_TESTING_PROVE(output.find("Configuration 1/2 completed") != std::string::npos);
                ARIADNE_TESTING_PROVE(output.find("(warm, warmup) started") != std::string::npos);
                ARIADNE_TESTING_PROVE(output.find("(warm, warmup) completed") != std::string::npos);
                ARIADNE_TESTING_PROVE(output.find("(measured) started") != std::string::npos);
                ARIADNE_TESTING_PROVE(output.find("(measured) completed") != std::string::npos);
                ARIADNE_TESTING_PROVE(output.find("elapsed_ns=12, normalized_ns=6") != std::string::npos);
            }
        }
        // Single-configuration progress never introduces a configuration line.
        capture.clear();
        RunLog single(1u);
        single.configuration_started(config);
        single.configuration_completed(config);
        ARIADNE_TESTING_PROVE(capture.str().empty());
    }

    void test_verbosity_depth() {
        RunLog single(1u);
        RunLog matrix(2u);
        // matrix adds exactly one detail level
        ARIADNE_TESTING_PROVE(single.execution_level() == 2u &&
                              single.measurement_level() == 3u &&
                              matrix.execution_level() == 3u &&
                              matrix.measurement_level() == 4u);
    }

    void test_warmups_are_verified_and_excluded_from_statistics() {
        unsigned prepared = 0u;
        unsigned executed = 0u;
        unsigned verified = 0u;
        unsigned value = 0u;
        Runner runner(2u);
        const Samples samples = runner.run(
            "work", 3u,
            [&] { ++prepared; value = 0u; },
            [&] { ++executed; value = 7u; },
            [&] {
                ++verified;
                if (value != 7u)
                    throw std::runtime_error("invalid benchmark result");
            });
        // warmups run full prepare, measure and verify phases
        ARIADNE_TESTING_PROVE(prepared == 5u && executed == 5u && verified == 5u);
        // only measured executions are sampled
        ARIADNE_TESTING_PROVE(samples.size() == 3u);
    }

    void test_default_runner_performs_no_warmups() {
        Runner runner;
        unsigned executed = 0u;
        const Samples samples = runner.run(
            "work", 2u,
            [] {},
            [&] { ++executed; },
            [] {});
        // zero warmup default preserves sample count
        ARIADNE_TESTING_PROVE(runner.warmups() == 0u && executed == 2u &&
                              samples.size() == 2u);
    }

    void test_warmup_verification_failures_propagate() {
        Runner runner(1u);
        unsigned executed = 0u;
        ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::runtime_error,
                                              runner.run("invalid", 1u, [] {}, [&] { ++executed; }, [&] { throw std::runtime_error("verification failed"); }), "verification failed");
        // invalid warmup aborts rather than silently discarding
        ARIADNE_TESTING_EXPECT_EQUAL(executed, 1u);
    }
};

int main(int argc, char** argv) {
    ARIADNE_TESTING_SUITE("Benchmarking runner");
    ARIADNE_TESTING_ADD_TEST(TestRunner, test_runner_validation_in_one_instantiation);
    ARIADNE_TESTING_ADD_TEST(TestRunner, test_progress_logging);
    ARIADNE_TESTING_ADD_TEST(TestRunner, test_verbosity_depth);
    ARIADNE_TESTING_ADD_TEST(TestRunner, test_warmups_are_verified_and_excluded_from_statistics);
    ARIADNE_TESTING_ADD_TEST(TestRunner, test_default_runner_performs_no_warmups);
    ARIADNE_TESTING_ADD_TEST(TestRunner, test_warmup_verification_failures_propagate);
    return ARIADNE_TESTING_RUN_TESTS_WITH_ARGS(argc, argv);
}

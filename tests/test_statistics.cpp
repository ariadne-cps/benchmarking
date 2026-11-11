/***************************************************************************
 *            test_statistics.cpp
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

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <limits>

using namespace Ariadne::Benchmarking;
using namespace Ariadne::Testing;

class TestStatistics : public ContextFixture {
public:
    using ContextFixture::ContextFixture;

    void test_calibration_boundaries() {
        double const inf = std::numeric_limits<double>::infinity();
        double const nan = std::numeric_limits<double>::quiet_NaN();
        struct Input { double pilot, minimum; unsigned repetitions, maximum; };
        for (auto const& input : std::vector<Input>{
                {inf, 1.0, 1u, 10u}, {nan, 1.0, 1u, 10u},
                {0.0, 1.0, 1u, 10u}, {-1.0, 1.0, 1u, 10u},
                {1.0, inf, 1u, 10u}, {1.0, nan, 1u, 10u},
                {1.0, 0.0, 1u, 10u}, {1.0, -1.0, 1u, 10u},
                {1.0, 1.0, 0u, 10u}, {1.0, 1.0, 1u, 0u}}) {
            auto scope = ctx().scope("calibration", std::to_string(input.pilot) +
                "/" + std::to_string(input.minimum) + "/" +
                std::to_string(input.repetitions) + "/" + std::to_string(input.maximum));
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                calibrated_repetitions(input.pilot, input.minimum,
                                       input.repetitions, input.maximum));
        }
        ARIADNE_TESTING_EXPECT_THROWS(std::overflow_error,
            calibrated_repetitions(std::numeric_limits<double>::min(),
                                   std::numeric_limits<double>::max(), 1u, 10u));
        // A positive requested duration may underflow in the division.
        ARIADNE_TESTING_EXPECT_EQUAL(calibrated_repetitions(
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::denorm_min(), 1u, 10u), 1u);
        ARIADNE_TESTING_EXPECT_EQUAL(calibrated_repetitions(3.0, 10.0, 2u, 20u), 7u);
    }

    void test_invalid_ratios_and_samples() {
        for (double invalid : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::quiet_NaN()}) {
            auto scope = ctx().scope("invalid timing", invalid);
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, speedup(invalid, 1.0));
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, speedup(1.0, invalid));
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, geometric_mean({invalid}));
        }
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, geometric_mean({}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, median_of_sorted({}));
        Samples samples;
        for (double invalid : {std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::quiet_NaN()}) {
            auto scope = ctx().scope("nonfinite sample", invalid);
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, samples.add(invalid));
        }
        samples.add(0.0);
        ARIADNE_TESTING_EXPECT_EQUAL(samples.values(), std::vector<double>({0.0}));
    }

    void test_millisecond_and_elapsed_timing() {
        unsigned calls = 0u;
        double const milliseconds = measure_ms([&] { ++calls; });
        ARIADNE_TESTING_EXPECT_EQUAL(calls, 1u);
        ARIADNE_TESTING_PROVE(milliseconds >= 0.0);
        auto const start = Clock::time_point{};
        auto const stop = start + std::chrono::milliseconds(2);
        ARIADNE_TESTING_EXPECT_EQUAL(elapsed_ns(start, stop), 2000000.0);
        ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::runtime_error,
            measure_ms([] { throw std::runtime_error("timed failure"); }), "timed failure");
    }

    void test_statistics() {
        Samples samples;
        for (double n : {1.0, 2.0, 3.0, 4.0}) samples.add(n);
        auto const result = summarize(samples);
        // summary of even samples with sample dispersion
        ARIADNE_TESTING_PROVE(result.count == 4 && result.median == 2.5
                  && result.mean == 2.5 && result.minimum == 1.0
                  && result.maximum == 4.0 && result.p95 == 4.0
                  && result.mad == 1.0
                  && std::abs(result.variance - 5.0/3.0) < 1e-12
                  && std::abs(result.stddev - std::sqrt(5.0/3.0)) < 1e-12);
    }

    void test_stable_sample_dispersion() {
        Samples samples;
        samples.add(1000000001.0);
        samples.add(1000000002.0);
        samples.add(1000000003.0);
        auto const stats = summarize(samples);
        // sample variance stable for large offsets
        ARIADNE_TESTING_PROVE(stats.mean == 1000000002.0 &&
                  std::abs(stats.variance - 1.0) < 1e-12 &&
                  std::abs(stats.stddev - 1.0) < 1e-12);
    }

    void test_calibration_and_ratios() {
        // calibrated repetitions
        ARIADNE_TESTING_PROVE(calibrated_repetitions(10.0, 100.0, 1u, 100u) == 10u);
        // bounded calibration
        ARIADNE_TESTING_PROVE(calibrated_repetitions(10.0, 1000.0, 1u, 15u) == 15u);
        // speedup convention
        ARIADNE_TESTING_PROVE(speedup(8.0, 4.0) == 2.0);
        // geometric mean
        ARIADNE_TESTING_PROVE(std::abs(geometric_mean({2.0, 8.0}) - 4.0) < 1e-12);
    }

    void test_invalid_input() {
        // empty samples rejected
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            summarize(Samples{}));
        // negative unsigned rejected
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            parse_unsigned<unsigned>(Ariadne::String("-1")));
        // negative timings rejected
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            Samples samples; samples.add(-1.0));
    }

    void test_timing_boundaries() {
        int invocations = 0;
        double const elapsed = measure_ns([&] { ++invocations; });
        // exactly one invocation
        ARIADNE_TESTING_PROVE(invocations == 1 && elapsed >= 0.0);
    }

};

int main(int argc, char** argv) {
    ARIADNE_TESTING_SUITE("Benchmarking statistics");
    ARIADNE_TESTING_ADD_TEST(TestStatistics, test_calibration_boundaries);
    ARIADNE_TESTING_ADD_TEST(TestStatistics, test_invalid_ratios_and_samples);
    ARIADNE_TESTING_ADD_TEST(TestStatistics, test_millisecond_and_elapsed_timing);
    ARIADNE_TESTING_ADD_TEST(TestStatistics, test_statistics);
    ARIADNE_TESTING_ADD_TEST(TestStatistics, test_stable_sample_dispersion);
    ARIADNE_TESTING_ADD_TEST(TestStatistics, test_calibration_and_ratios);
    ARIADNE_TESTING_ADD_TEST(TestStatistics, test_invalid_input);
    ARIADNE_TESTING_ADD_TEST(TestStatistics, test_timing_boundaries);
    return ARIADNE_TESTING_RUN_TESTS_WITH_ARGS(argc, argv);
}

/***************************************************************************
 *            test_parameters.cpp
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

#include "ariadne-benchmarking.hpp"
#include "ariadne-testing.hpp"

using namespace Ariadne::Benchmarking;
using namespace Ariadne::Testing;

class TestParameters : public ContextFixture {
  public:
    using ContextFixture::ContextFixture;

    void test_quoted_and_escaped_values() {
        ARIADNE_TESTING_EXPECT_EQUAL(detail::split_values("a\\,b,\"\",c\\\\d"),
                                     std::vector<std::string>({"a,b", "", "c\\d"}));
        ARIADNE_TESTING_EXPECT_EQUAL(detail::split_values("\"\""),
                                     std::vector<std::string>({""}));
        for (const char* input: {"", " ", "1,", "unfinished\\"}) {
            auto scope = ctx().scope("invalid value list", input);
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, detail::split_values(input));
        }
        ARIADNE_TESTING_EXPECT_EQUAL(detail::quote_if_needed("a,b"), "\"a,b\"");
        for (const char* input: {"", "-1", "bad", "1tail"}) {
            auto scope = ctx().scope("invalid numeric parameter", input);
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                          detail::parse_value<unsigned>(input));
        }
        ARIADNE_TESTING_EXPECT_EQUAL(detail::parse_value<unsigned>("2 "), 2u);
        ARIADNE_TESTING_EXPECT_EQUAL(detail::parse_value<std::string>("a,b"), "a,b");
    }

    void test_parameter_contracts() {
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                      Parameter<unsigned>("", "Unnamed", {1u}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                      Parameter<unsigned>("x", "Empty", {}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                      Parameter<unsigned>("x", "No parser", {1u}, {}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                      Parameter<unsigned>("x", "No formatter", {1u}, detail::parse_value<unsigned>, {}));
        Parameter<unsigned> first("first", "First", {2u});
        Parameter<unsigned> second("second", "Second", {3u});
        Parameter<unsigned> absent("absent", "Absent", {4u});
        Parameter<unsigned> duplicate("first", "Duplicate name", {5u});
        Matrix matrix;
        matrix.add(first).add(second);
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, matrix.add(first));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, matrix.add(duplicate));
        matrix.for_each([&](const Configuration& config) {
            ARIADNE_TESTING_EXPECT_EQUAL(config.value(second), 3u);
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, config.value(absent));
        });
        ARIADNE_TESTING_EXPECT_THROWS(std::out_of_range, first.at(1u));
    }

    void test_string_parameter_cli() {
        Parameter<std::string> backend("backend", "Backend", {"default"});
        Matrix matrix;
        matrix.add(backend);
        CommandLine cli;
        cli.add_parameters(matrix);
        const char* argv[] = {"benchmark", "--backend", "fast,\"b,c\",\"\""};
        ARIADNE_TESTING_PROVE(cli.acquire(arguments(3, argv)));
        ARIADNE_TESTING_EXPECT_EQUAL(backend.values(),
                                     std::vector<std::string>({"fast", "b,c", ""}));
        ARIADNE_TESTING_EXPECT_EQUAL(matrix.combinations(), 3u);
    }

    void test_empty_matrix() {
        Matrix matrix;
        ARIADNE_TESTING_EXPECT_EQUAL(matrix.header(), "Parameters: (none) | configurations=1");
        unsigned calls = 0u;
        matrix.for_each([&](const Configuration& config) {
            ++calls;
            ARIADNE_TESTING_EXPECT_EQUAL(config.ordinal(), 1u);
            ARIADNE_TESTING_EXPECT_EQUAL(config.count(), 1u);
            ARIADNE_TESTING_PROVE(config.label().empty());
            ARIADNE_TESTING_PROVE(config.dimensions().empty());
            ARIADNE_TESTING_EXPECT_EQUAL(ComparisonContext::from(config).label, "default");
        });
        ARIADNE_TESTING_EXPECT_EQUAL(calls, 1u);
    }

    void test_lazy_matrix_cardinality_limits() {
        // A lazy axis exposes cardinality without allocating its entire domain.
        class LazyAxis : public ParameterBase {
          public:
            LazyAxis(std::string name, std::size_t count)
                : ParameterBase(std::move(name), "Lazy index range"), _count(count) {}
            std::size_t size() const override { return _count; }
            std::string display_at(std::size_t index) const override {
                return std::to_string(index);
            }
            Ariadne::ArgumentParser make_parser() override {
                throw std::logic_error("fixed cardinality axis has no CLI parser");
            }

          private:
            std::size_t _count;
        };
        LazyAxis empty("empty", 0u);
        Matrix empty_matrix;
        empty_matrix.add(empty);
        ARIADNE_TESTING_EXPECT_THROWS(std::overflow_error, empty_matrix.combinations());
        LazyAxis huge("huge", std::numeric_limits<std::size_t>::max());
        LazyAxis two("two", 2u);
        Matrix matrix;
        matrix.add(huge);
        ARIADNE_TESTING_EXPECT_EQUAL(matrix.combinations(), std::numeric_limits<std::size_t>::max());
        matrix.add(two);
        ARIADNE_TESTING_EXPECT_THROWS(std::overflow_error, matrix.combinations());
    }

    void test_benchmark_parameter_matrix() {
        Parameter<unsigned> size("size", "Size", {2u, 4u});
        Parameter<std::string> backend("backend", "Backend", {"fast", "slow"});
        Matrix matrix;
        matrix.add(size).add(backend);
        std::vector<std::string> seen;
        matrix.for_each([&](const Configuration& config) {
            seen.push_back(config.label());
            // typed parameter lookup
            ARIADNE_TESTING_PROVE(config.value(size) == 2u || config.value(size) == 4u);
        });
        // Cartesian product order
        ARIADNE_TESTING_PROVE(matrix.combinations() == 4u &&
                              seen == std::vector<std::string>({"size=2, backend=fast", "size=2, backend=slow",
                                                                "size=4, backend=fast", "size=4, backend=slow"}));
        // quote only separators and whitespace
        ARIADNE_TESTING_PROVE(detail::quote_if_needed("backend") == "backend" &&
                              detail::quote_if_needed("fast") == "fast" &&
                              detail::quote_if_needed("hello world") == std::string(1, '"') + "hello world" + '"');
        // all parameter sequences listed in header
        ARIADNE_TESTING_PROVE(matrix.header() ==
                              "Parameters: size=[2, 4]; backend=[fast, slow] | configurations=4");
    }

    void test_list_cli_parsing_and_duplicates() {
        Parameter<unsigned> size("size", "Size", {7u});
        Matrix matrix;
        matrix.add(size);
        CommandLine cli;
        cli.add_parameters(matrix);
        Ariadne::List<Ariadne::String> args;
        args.append(Ariadne::String("test_benchmarking"));
        args.append(Ariadne::String("--size"));
        args.append(Ariadne::String("2, 3,2"));
        const bool accepted = cli.acquire(args);
        // CLI retains order and duplicate values
        ARIADNE_TESTING_PROVE(accepted &&
                              size.values() == std::vector<unsigned>({2u, 3u, 2u}));
        // empty list members rejected
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                      detail::split_values("1,,3"));
        // unterminated quoted value rejected
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                      detail::split_values("\"unterminated"));
        // quoted comma preserved in a string value
        ARIADNE_TESTING_PROVE(detail::split_values("a,\"b,c\",d") ==
                              std::vector<std::string>({"a", "b,c", "d"}));
    }
};

int main(int argc, char** argv) {
    ARIADNE_TESTING_SUITE("Benchmarking parameters");
    ARIADNE_TESTING_ADD_TEST(TestParameters, test_quoted_and_escaped_values);
    ARIADNE_TESTING_ADD_TEST(TestParameters, test_parameter_contracts);
    ARIADNE_TESTING_ADD_TEST(TestParameters, test_string_parameter_cli);
    ARIADNE_TESTING_ADD_TEST(TestParameters, test_empty_matrix);
    ARIADNE_TESTING_ADD_TEST(TestParameters, test_lazy_matrix_cardinality_limits);
    ARIADNE_TESTING_ADD_TEST(TestParameters, test_benchmark_parameter_matrix);
    ARIADNE_TESTING_ADD_TEST(TestParameters, test_list_cli_parsing_and_duplicates);
    return ARIADNE_TESTING_RUN_TESTS_WITH_ARGS(argc, argv);
}

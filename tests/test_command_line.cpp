/***************************************************************************
 *            test_command_line.cpp
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

class TestCommandLine : public ContextFixture {
  public:
    using ContextFixture::ContextFixture;

    void test_integer_parsing() {
        for (const char* text: {"", "-1", "12x", "4294967296"}) {
            auto scope = ctx().scope("invalid unsigned", text);
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                          parse_unsigned<unsigned>(Ariadne::String(text)));
        }
        ARIADNE_TESTING_EXPECT_THROWS(std::out_of_range,
                                      parse_unsigned<unsigned>(Ariadne::String("18446744073709551616")));
        ARIADNE_TESTING_EXPECT_EQUAL(parse_unsigned<unsigned>(Ariadne::String("0")), 0u);
        ARIADNE_TESTING_EXPECT_EQUAL(parse_unsigned<unsigned>(Ariadne::String("4294967295")),
                                     std::numeric_limits<unsigned>::max());
        for (const char* text: {"", ",1", "1,", "1,,2", "1,1", "0,2"}) {
            auto scope = ctx().scope("invalid unique list", text);
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
                                          parse_unique_unsigned_list(Ariadne::String(text), 1u));
        }
        ARIADNE_TESTING_EXPECT_EQUAL(parse_unique_unsigned_list(Ariadne::String("0,2,1")),
                                     std::vector<unsigned>({0u, 2u, 1u}));
        ARIADNE_TESTING_EXPECT_EQUAL(parse_unique_unsigned_list(Ariadne::String("3"), 3u),
                                     std::vector<unsigned>({3u}));
    }

    void test_argument_conversion() {
        const char* argv[] = {"benchmark", "--warmups", "3"};
        const auto args = arguments(3, argv);
        ARIADNE_TESTING_REQUIRE(args.size() == 3u);
        ARIADNE_TESTING_EXPECT_EQUAL(args[0], Ariadne::String("benchmark"));
        ARIADNE_TESTING_EXPECT_EQUAL(args[1], Ariadne::String("--warmups"));
        ARIADNE_TESTING_EXPECT_EQUAL(args[2], Ariadne::String("3"));
        ARIADNE_TESTING_PROVE(arguments(0, nullptr).empty());
    }

    void test_summary_field_validation() {
        const auto invalid = static_cast<SummaryField>(-1);
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, summary_field_id(invalid));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, summary_field_heading(invalid));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, parse_summary_fields(" \t "));
    }

    void test_summary_columns_and_cli_selection() {
        // default summary columns and order
        ARIADNE_TESTING_PROVE(default_summary_fields() ==
                              SummaryFields({SummaryField::Minimum, SummaryField::Maximum,
                                             SummaryField::Mean, SummaryField::MAD}));
        // custom order preserved
        ARIADNE_TESTING_PROVE(parse_summary_fields(" stddev, min , count ") ==
                              SummaryFields({SummaryField::Stddev, SummaryField::Minimum,
                                             SummaryField::Count}));
        for (const char* invalid: {"", "min,", ",max", "min,,max",
                                   "median,median", "unknown"}) {
            auto scope = ctx().scope("invalid summary", invalid);
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, parse_summary_fields(invalid));
        }
        CommandLine cli;
        Ariadne::List<Ariadne::String> args;
        args.append(Ariadne::String("test_benchmarking"));
        args.append(Ariadne::String("--summary"));
        args.append(Ariadne::String("max,mean,variance,stddev"));
        const bool accepted = cli.acquire(args);
        // CLI applies a custom summary column selection
        ARIADNE_TESTING_PROVE(accepted && cli.summary_fields() ==
                                              SummaryFields({SummaryField::Maximum, SummaryField::Mean,
                                                             SummaryField::Variance, SummaryField::Stddev}));
    }

    void test_logging_without_threading() {
        CommandLine cli;
        Ariadne::List<Ariadne::String> args;
        args.append(Ariadne::String("test_benchmarking"));
        args.append(Ariadne::String("--scheduler"));
        args.append(Ariadne::String("blocking"));
        const bool acquired = cli.acquire(args);
        ARIADNE_TESTING_PROVE(acquired);
        ARIADNE_TESTING_PROVE(not Ariadne::Logger::instance().has_thread_registry_attached());
        Ariadne::Logger::instance().use_nonblocking_scheduler();
    }

    void test_global_warmup_cli() {
        CommandLine defaults;
        // default warmup count is zero
        ARIADNE_TESTING_PROVE(defaults.warmups() == 0u);
        CommandLine cli;
        Ariadne::List<Ariadne::String> args;
        args.append(Ariadne::String("test_benchmarking"));
        args.append(Ariadne::String("--warmups"));
        args.append(Ariadne::String("2"));
        const bool accepted = cli.acquire(args);
        // global warmup CLI accepted
        ARIADNE_TESTING_PROVE(accepted && cli.warmups() == 2u);
        Runner configured(cli);
        // runner inherits global warmup count
        ARIADNE_TESTING_PROVE(configured.warmups() == 2u);
    }
};

int main(int argc, char** argv) {
    ARIADNE_TESTING_SUITE("Benchmarking command_line");
    ARIADNE_TESTING_ADD_TEST(TestCommandLine, test_integer_parsing);
    ARIADNE_TESTING_ADD_TEST(TestCommandLine, test_argument_conversion);
    ARIADNE_TESTING_ADD_TEST(TestCommandLine, test_summary_field_validation);
    ARIADNE_TESTING_ADD_TEST(TestCommandLine, test_summary_columns_and_cli_selection);
    ARIADNE_TESTING_ADD_TEST(TestCommandLine, test_logging_without_threading);
    ARIADNE_TESTING_ADD_TEST(TestCommandLine, test_global_warmup_cli);
    return ARIADNE_TESTING_RUN_TESTS_WITH_ARGS(argc, argv);
}

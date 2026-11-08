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

class TestReporting : public ContextFixture {
public:
    using ContextFixture::ContextFixture;

    void test_statistic_column_values() {
        Report report("Every statistic", {SummaryField::Minimum, SummaryField::Maximum,
            SummaryField::Mean, SummaryField::MAD, SummaryField::Median, SummaryField::P95,
            SummaryField::Stddev, SummaryField::Variance, SummaryField::Count});
        report.set_dimension_columns({{"Case"}});
        TableOptions options;
        options.separator = "|";
        report.set_table_options(options);
        Statistics const stats{5.0, 3.0, 1.0, 2.0, 4.0, 6.0, 9u, 8.0, 7.0};
        report.add_row({"case"}, stats);
        auto const table = report.table();
        std::istringstream row(table.substr(table.find('\n') + 1u));
        std::vector<std::string> cells;
        std::string cell;
        while (std::getline(row, cell, '|')) cells.push_back(detail::trim(cell));
        ARIADNE_TESTING_EXPECT_EQUAL(cells, std::vector<std::string>({
            "case", "1.000", "2.000", "3.000", "4.000", "5.000", "6.000", "7.000", "8.000", "9"}));
    }

    void test_report_logging() {
        LogCapture capture;
        Report report("Measured results");
        for (unsigned verbosity : {0u, 1u}) {
            auto scope = ctx().scope("verbosity", verbosity);
            capture.verbosity(verbosity);
            capture.clear();
            report.set_parameter_header("");
            report.print();
            report.set_parameter_header("Parameters: size=[2]");
            report.print();
            if (verbosity == 0u) {
                ARIADNE_TESTING_PROVE(capture.str().empty());
            } else {
                ARIADNE_TESTING_PROVE(capture.str().find("Measured results") != std::string::npos);
                ARIADNE_TESTING_PROVE(capture.str().find("Parameters: size=[2]") != std::string::npos);
                ARIADNE_TESTING_PROVE(capture.str().find("Configuration") != std::string::npos);
            }
        }
    }

    void test_table_validation_and_overflow() {
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, Table({}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, Table({{"bad\nheading"}}));
        TableOptions options;
        options.group_columns = {1u};
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, Table({{"one"}}, options));
        options = {};
        options.separator = "\n";
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, Table({{"one"}}, options));
        for (char invalid : {'\r', '\n'}) {
            auto scope = ctx().scope("invalid rule", static_cast<int>(invalid));
            options = {};
            options.header_rule = invalid;
            ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, Table({{"one"}}, options));
        }
        Table table({{"one"}});
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, table.add_row({}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, table.add_row({"bad\rvalue"}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, Table::number(1.0, 31u));
        ARIADNE_TESTING_EXPECT_EQUAL(Table::number(2.5, 1u, "x"), "2.5x");
        auto const max = std::numeric_limits<std::size_t>::max();
        Table wide({{"", max}, {"x"}});
        ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::overflow_error, wide.str(), "table width");
        Table spaced({{"", max}, {""}});
        ARIADNE_TESTING_EXPECT_THROWS_MESSAGE(std::overflow_error, spaced.str(), "separator width");
    }

    void test_report_validation() {
        Report report("Validation");
        ARIADNE_TESTING_PROVE(report.summary_fields() == default_summary_fields());
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, report.set_summary_fields({}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            report.set_summary_fields({SummaryField::Mean, SummaryField::Mean}));
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument,
            report.set_summary_fields({static_cast<SummaryField>(-1)}));
        ARIADNE_TESTING_PROVE(report.summary_fields() == default_summary_fields());
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, report.set_dimension_columns({}));
        Samples samples;
        samples.add(10.0);
        auto const stats = summarize(samples);
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, report.add_row({"one"}, stats));
        report.add("config", "op", stats);
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, report.set_dimension_columns({{"one"}}));
        report.set_dimension_columns({{"Case"}, {"Action"}});
        ARIADNE_TESTING_PROVE(report.table().find("Action") != std::string::npos);
        TableOptions options;
        options.group_columns = {2u};
        report.set_table_options(options);
        ARIADNE_TESTING_EXPECT_THROWS(std::invalid_argument, report.table());
    }

    void test_report_output_and_single_sample_dispersion() {
        Samples samples;
        samples.add(10.0);
        auto const stats = summarize(samples);
        // single sample dispersion is undefined
        ARIADNE_TESTING_PROVE(std::isnan(stats.variance) && std::isnan(stats.stddev));
        Report defaults("Default");
        defaults.add("one", "operation", stats);
        std::string const header = defaults.table().substr(
            0u, defaults.table().find('\n'));
        auto const min_pos = header.find("Min (ns)");
        auto const max_pos = header.find("Max (ns)");
        auto const mean_pos = header.find("Mean (ns)");
        auto const mad_pos = header.find("MAD (ns)");
        // default table only prints min max mean MAD in order
        ARIADNE_TESTING_PROVE(min_pos != std::string::npos && min_pos < max_pos &&
                  max_pos < mean_pos && mean_pos < mad_pos &&
                  header.find("Median (ns)") == std::string::npos);
        Report custom("Custom", parse_summary_fields(
            "count,stddev,variance,median,p95"));
        custom.add("one", "operation", stats);
        std::string const table = custom.table();
        // custom report labels and undefined dispersion rendered
        ARIADNE_TESTING_PROVE(table.find("Variance (ns^2)") != std::string::npos &&
                  table.find("Stddev (ns)") != std::string::npos &&
                  table.find("n/a") != std::string::npos &&
                  table.find("Median (ns)") != std::string::npos);
    }

    void test_table_widths_use_actual_displayed_values() {
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
        // heading contains dimensions and measures
        ARIADNE_TESTING_PROVE(header.find("Operation") != std::string::npos &&
                  header.find("Median (ns)") != std::string::npos);
        // columns use longest rendered cell, including long operation
        ARIADNE_TESTING_PROVE(short_row.size() == long_row.size() &&
                  short_row.size() == header.size());
        // numeric columns preserve formatted content
        ARIADNE_TESTING_PROVE(long_row.find("1234567.125") != std::string::npos &&
                  short_row.find("2.500") != std::string::npos);
        // outer rule spans computed table width
        ARIADNE_TESTING_PROVE(result.substr(0u, first_end) ==
                  std::string(header.size(), '='));
    }

    void test_group_separators_and_optional_minimum_widths() {
        auto options = TableOptions::ruled();
        options.group_columns = {0u};
        Table grouped({{"Backend"}, {"Operation"}, {"Bits"}}, options);
        grouped.add_row({"floatdp", "add", "53"});
        grouped.add_row({"floatdp", "sub", "53"});
        grouped.add_row({"floatmp", "add", "53"});
        auto const output = grouped.str();
        auto const row_separator = output.find("\n-", output.find("sub"));
        // group separator inserted when backend changes
        ARIADNE_TESTING_PROVE(row_separator != std::string::npos &&
                  row_separator < output.find("floatmp", row_separator));
        Table padded({{"A", 10u}, {"B"}});
        padded.add_row({"x", "value"});
        // explicit minimum width supported but not imposed by default
        ARIADNE_TESTING_PROVE(padded.str().find("x         ") != std::string::npos);
    }

    void test_custom_report_dimensions_and_computed_numeric_widths() {
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
        // descriptive and statistical columns share a generic table
        ARIADNE_TESTING_PROVE(output.find("Backend") != std::string::npos &&
                  output.find("Operation") != std::string::npos &&
                  output.find("Bits") != std::string::npos &&
                  output.find("Median (ns)") != std::string::npos);
        // long numeric and descriptive values are fully displayed
        ARIADNE_TESTING_PROVE(output.find("123456789.000") != std::string::npos &&
                  output.find("long_operation_name") != std::string::npos);
    }

};

int main(int argc, char** argv) {
    ARIADNE_TESTING_SUITE("Benchmarking reporting");
    ARIADNE_TESTING_ADD_TEST(TestReporting, test_statistic_column_values);
    ARIADNE_TESTING_ADD_TEST(TestReporting, test_report_logging);
    ARIADNE_TESTING_ADD_TEST(TestReporting, test_table_validation_and_overflow);
    ARIADNE_TESTING_ADD_TEST(TestReporting, test_report_validation);
    ARIADNE_TESTING_ADD_TEST(TestReporting, test_report_output_and_single_sample_dispersion);
    ARIADNE_TESTING_ADD_TEST(TestReporting, test_table_widths_use_actual_displayed_values);
    ARIADNE_TESTING_ADD_TEST(TestReporting, test_group_separators_and_optional_minimum_widths);
    ARIADNE_TESTING_ADD_TEST(TestReporting, test_custom_report_dimensions_and_computed_numeric_widths);
    return ARIADNE_TESTING_RUN_TESTS_WITH_ARGS(argc, argv);
}

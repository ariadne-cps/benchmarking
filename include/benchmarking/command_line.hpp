#ifndef ARIADNE_BENCHMARKING_COMMAND_LINE_HPP
#define ARIADNE_BENCHMARKING_COMMAND_LINE_HPP

#include "benchmarking/logging.hpp"
#include "benchmarking/parameters.hpp"
#include "benchmarking/summary_fields.hpp"
#include "threading/threading_command_line_interface.hpp"
#include "utility/command_line_interface.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace Ariadne::Benchmarking {

template<class UInt>
UInt parse_unsigned(String const& value) {
    std::string const text(value);
    if (text.empty() or text.front() == '-')
        throw std::invalid_argument("expected nonnegative integer");
    std::size_t used = 0u;
    auto const parsed = std::stoull(text, &used);
    if (used != text.size() or parsed > std::numeric_limits<UInt>::max())
        throw std::invalid_argument("integer out of range");
    return static_cast<UInt>(parsed);
}

inline std::vector<unsigned> parse_unique_unsigned_list(
        String const& value, unsigned minimum = 0u) {
    std::string const text(value);
    if (text.empty() or text.front() == ',' or text.back() == ',')
        throw std::invalid_argument("invalid integer list");
    std::vector<unsigned> result;
    std::size_t start = 0u;
    do {
        std::size_t end = text.find(',', start);
        unsigned const number = parse_unsigned<unsigned>(
            String(text.substr(start, end == std::string::npos
                              ? end : end - start)));
        if (number < minimum or
            std::find(result.begin(), result.end(), number) != result.end())
            throw std::invalid_argument("invalid or duplicate integer");
        result.push_back(number);
        if (end == std::string::npos) break;
        start = end + 1u;
    } while (true);
    return result;
}

inline List<String> arguments(int argc, char const* argv[]) {
    List<String> result;
    for (int i = 0; i < argc; ++i) result.append(String(argv[i]));
    return result;
}

class CommandLine : public ThreadingCommandLineInterface {
public:
    CommandLine() {
        initialize_logging();
        add(CommandLineContribution("benchmarking", {
            make_valued_argument_parser(
                "", "summary",
                "Summary columns in order (default: min,max,mean,mad). "
                "Available: min,max,mean,mad,median,p95,stddev,variance,count",
                [this](String const& text) -> VoidFunction {
                    SummaryFields fields = parse_summary_fields(std::string(text));
                    return [this, fields = std::move(fields)]() mutable {
                        _summary_fields = std::move(fields);
                    };
                }),
            make_valued_argument_parser(
                "", "warmups",
                "Warm-up executions per configuration (default: 0; excluded from statistics)",
                [this](String const& text) -> VoidFunction {
                    unsigned const count = parse_unsigned<unsigned>(text);
                    return [this, count]() { _warmups = count; };
                })
        }));
    }

    SummaryFields const& summary_fields() const { return _summary_fields; }
    unsigned warmups() const { return _warmups; }

    void add_parameters(Matrix& matrix) {
        add(CommandLineContribution("benchmark-parameters", matrix.parsers()));
    }

private:
    SummaryFields _summary_fields = default_summary_fields();
    unsigned _warmups = 0u;
};
} // namespace Ariadne::Benchmarking

#endif

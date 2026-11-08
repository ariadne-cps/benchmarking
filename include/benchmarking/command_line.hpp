#ifndef ARIADNE_BENCHMARKING_COMMAND_LINE_HPP
#define ARIADNE_BENCHMARKING_COMMAND_LINE_HPP

#include "benchmarking/logging.hpp"
#include "logging/logging_command_line_interface.hpp"
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

class CommandLine : public LoggingCommandLineInterface {
public:
    CommandLine() { initialize_logging(); }
};
} // namespace Ariadne::Benchmarking

#endif

#ifndef ARIADNE_BENCHMARKING_RUN_LOGGING_HPP
#define ARIADNE_BENCHMARKING_RUN_LOGGING_HPP

#include "benchmarking/parameters.hpp"
#include "logging/logging.hpp"
#include <cstddef>
#include <string>

namespace Ariadne::Benchmarking {

// Level 1 is reserved for Report's overall summary and the parameter header.
// A matrix introduces one extra depth for execution and measurement messages.
class RunLog {
public:
    explicit RunLog(std::size_t configurations) : _matrix(configurations > 1u) {}

    void configuration_started(Configuration const& config) const {
        if (_matrix)
            LOGGING_PRINTLN_AT(2, "Configuration " << config.ordinal()
                << '/' << config.count() << " started: " << config.label())
    }
    void configuration_completed(Configuration const& config) const {
        if (_matrix)
            LOGGING_PRINTLN_AT(2, "Configuration " << config.ordinal()
                << '/' << config.count() << " completed: " << config.label())
    }
    void execution_started(std::string const& name, std::size_t ordinal,
                           bool warmup = false) const {
        LOGGING_PRINTLN_AT(execution_level(), "Execution " << ordinal << " ("
            << name << (warmup ? ", warmup" : "") << ") started")
    }
    void execution_completed(std::string const& name, std::size_t ordinal,
                             bool warmup = false) const {
        LOGGING_PRINTLN_AT(execution_level(), "Execution " << ordinal << " ("
            << name << (warmup ? ", warmup" : "") << ") completed")
    }
    void measurement(std::string const& name, std::size_t ordinal,
                     double elapsed_ns, double normalized_ns) const {
        LOGGING_PRINTLN_AT(measurement_level(), "Execution " << ordinal
            << " (" << name << "): elapsed_ns=" << elapsed_ns
            << ", normalized_ns=" << normalized_ns)
    }

    unsigned execution_level() const { return _matrix ? 3u : 2u; }
    unsigned measurement_level() const { return _matrix ? 4u : 3u; }

private:
    bool _matrix;
};

} // namespace Ariadne::Benchmarking

#endif

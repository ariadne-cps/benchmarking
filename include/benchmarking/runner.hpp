#ifndef ARIADNE_BENCHMARKING_RUNNER_HPP
#define ARIADNE_BENCHMARKING_RUNNER_HPP

#include "benchmarking/command_line.hpp"
#include "benchmarking/run_logging.hpp"
#include "benchmarking/samples.hpp"
#include "benchmarking/timing.hpp"

#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace Ariadne::Benchmarking {

// Implements the global warm-up policy independently of the parameter matrix.
// One warm-up is one full execution of operation, including preparation and
// validation, but its duration is NOT included in the returned Samples.
//
// The caller explicitly controls what happens before, during and after timing:
// prepare() -- outside the timer; operation() -- timed; verify() -- outside.
// Stateful operations can therefore restore their input before each round.
class Runner {
public:
    explicit Runner(CommandLine const& command_line,
                    std::size_t configurations = 1u)
        : Runner(command_line.warmups(), configurations) {}

    explicit Runner(unsigned warmups = 0u, std::size_t configurations = 1u)
        : _warmups(warmups), _log(configurations) {}

    unsigned warmups() const { return _warmups; }

    void configuration_started(Configuration const& configuration) const {
        _log.configuration_started(configuration);
    }

    void configuration_completed(Configuration const& configuration) const {
        _log.configuration_completed(configuration);
    }

    // normalization divides each measured elapsed time, e.g. by the number
    // of repetitions inside operation(). It does not affect warm-up count.
    template<class Prepare, class Operation, class Verify>
    Samples run(std::string const& name, unsigned samples,
                Prepare&& prepare, Operation&& operation, Verify&& verify,
                double normalization = 1.0) const {
        if (samples == 0u)
            throw std::invalid_argument("measured samples must be positive");
        if (not std::isfinite(normalization) or normalization <= 0.0)
            throw std::invalid_argument("normalization must be finite and positive");
        if (samples > std::numeric_limits<unsigned>::max() - _warmups)
            throw std::overflow_error("warm-up and sample count overflow");

        Samples result;
        unsigned const total = samples + _warmups;
        for (unsigned round = 0u; round < total; ++round) {
            bool const warmup = round < _warmups;
            std::size_t const ordinal = static_cast<std::size_t>(round) + 1u;
            _log.execution_started(name, ordinal, warmup);
            std::invoke(prepare);
            double const elapsed = measure_ns([&operation] {
                std::invoke(operation);
            });
            std::invoke(verify);
            if (not warmup) {
                double const normalized = elapsed / normalization;
                result.add(normalized);
                _log.measurement(name, ordinal, elapsed, normalized);
            }
            _log.execution_completed(name, ordinal, warmup);
        }
        return result;
    }

private:
    unsigned _warmups;
    RunLog _log;
};

} // namespace Ariadne::Benchmarking

#endif

/***************************************************************************
 *            benchmarking/runner.hpp
 *
 *  Copyright  2026  Luca Geretti
 *
 ****************************************************************************/

/*
 *  This file is part of Ariadne.
 *
 *  Ariadne is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Ariadne is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne.  If not, see <https://www.gnu.org/licenses/>.
 */

/*! \file benchmarking/runner.hpp
 *  \brief Execute benchmark rounds with explicit timed region boundaries.
 */

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
//! Run preparation, timed operation and verification for every round.
//! Warm-ups run and are validated but are omitted from the resulting Samples.
class Runner {
public:
    //! Initialize warm-up policy from CLI and configure matrix-aware logging.
    explicit Runner(CommandLine const& command_line,
                    std::size_t configurations = 1u)
        : Runner(command_line.warmups(), configurations) {}

    //! Initialize an explicitly configured warm-up count and matrix size.
    explicit Runner(unsigned warmups = 0u, std::size_t configurations = 1u)
        : _warmups(warmups), _log(configurations) {}

    //! Return the number of warm-up rounds before measured samples.
    unsigned warmups() const { return _warmups; }

    //! Log the beginning of a configuration.
    void configuration_started(Configuration const& configuration) const {
        _log.configuration_started(configuration);
    }

    //! Log the completion of a configuration.
    void configuration_completed(Configuration const& configuration) const {
        _log.configuration_completed(configuration);
    }

    // normalization divides each measured elapsed time, e.g. by the number
    // of repetitions inside operation(). It does not affect warm-up count.
    template<class Prepare, class Operation, class Verify>
    //! Measure samples rounds after warmups, returning normalized nanoseconds.
    //! @param prepare Executed before each round outside timing.
    //! @param operation Sole callback executed within the timer.
    //! @param verify Executed after each round outside timing, including warm-ups.
    //! @param normalization Positive finite divisor for each elapsed duration.
    //! @return Exactly samples normalized timings in nanoseconds.
    //! @throws std::invalid_argument for zero samples or invalid normalization.
    //! @note Callback exceptions propagate unchanged.
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

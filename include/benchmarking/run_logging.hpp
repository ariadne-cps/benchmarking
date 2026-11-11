/***************************************************************************
 *            benchmarking/run_logging.hpp
 *
 *  Copyright  2026  Luca Geretti
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

/** @file benchmarking/run_logging.hpp
 *  @brief Progress messages for measured and warm-up executions.
 */

#ifndef ARIADNE_BENCHMARKING_RUN_LOGGING_HPP
#define ARIADNE_BENCHMARKING_RUN_LOGGING_HPP

#include <cstddef>
#include <string>

#include "benchmarking/parameters.hpp"
#include "logging/logging.hpp"

namespace Ariadne::Benchmarking {

// Level 1 is reserved for Report's overall summary and the parameter header.
// A matrix introduces one extra depth for execution and measurement messages.
//! Choose logging detail based on the number of configurations.
//! With a matrix: configuration/execution/measurement use levels 2/3/4.
//! With one configuration: execution/measurement use levels 2/3.
class RunLog {
  public:
    //! Select the verbosity depth based on whether a matrix is in use.
    explicit RunLog(std::size_t configurations) : _matrix(configurations > 1u) {}

    //! Log the beginning of one matrix configuration.
    void configuration_started(const Configuration& config) const {
        if (_matrix)
            LOGGING_PRINTLN_AT(2, "Configuration " << config.ordinal()
                                                   << '/' << config.count() << " started: " << config.label())
    }
    //! Log the end of one matrix configuration.
    void configuration_completed(const Configuration& config) const {
        if (_matrix)
            LOGGING_PRINTLN_AT(2, "Configuration " << config.ordinal()
                                                   << '/' << config.count() << " completed: " << config.label())
    }
    //! Log the start of an operation round, marking a warm-up if applicable.
    void execution_started(const std::string& name, std::size_t ordinal,
                           bool warmup = false) const {
        LOGGING_PRINTLN_AT(execution_level(), "Execution " << ordinal << " ("
                                                           << name << (warmup ? ", warmup" : "") << ") started")
    }
    //! Log a round's completion, marking a warm-up if applicable.
    void execution_completed(const std::string& name, std::size_t ordinal,
                             bool warmup = false) const {
        LOGGING_PRINTLN_AT(execution_level(), "Execution " << ordinal << " ("
                                                           << name << (warmup ? ", warmup" : "") << ") completed")
    }
    //! Log elapsed and normalized measured durations in nanoseconds.
    void measurement(const std::string& name, std::size_t ordinal,
                     double elapsed_ns, double normalized_ns) const {
        LOGGING_PRINTLN_AT(measurement_level(), "Execution " << ordinal
                                                             << " (" << name << "): elapsed_ns=" << elapsed_ns
                                                             << ", normalized_ns=" << normalized_ns)
    }

    //! Return the verbosity used for execution events.
    unsigned execution_level() const { return _matrix ? 3u : 2u; }
    //! Return the verbosity used for individual measurements.
    unsigned measurement_level() const { return _matrix ? 4u : 3u; }

  private:
    bool _matrix;
};

} // namespace Ariadne::Benchmarking

#endif

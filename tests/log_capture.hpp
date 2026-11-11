/***************************************************************************
 *            tests/log_capture.hpp
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

/** @file tests/log_capture.hpp
 *  @brief Helpers for capturing and restoring logger output in benchmark tests.
 */

#ifndef ARIADNE_BENCHMARKING_TEST_LOG_CAPTURE_HPP
#define ARIADNE_BENCHMARKING_TEST_LOG_CAPTURE_HPP

#include <iostream>
#include <sstream>

#include "benchmarking/logging.hpp"

// These tests own the process logger and run without a ThreadManager. Capture
// its immediate output and restore formatting and the stream even on failure.
/** @brief Scoped capture of logger output for benchmark tests. */
class LogCapture {
  public:
    /** @brief Construct a \c LogCapture with the specified characteristics. */
    LogCapture()
        : _verbosity(Ariadne::Logger::instance().configuration().verbosity()),
          _theme(Ariadne::Logger::instance().configuration().theme()) {
        Ariadne::Logger::instance().use_immediate_scheduler();
        Ariadne::Logger::instance().configuration().set_theme(Ariadne::TT_THEME_NONE);
        _previous = std::clog.rdbuf(_output.rdbuf());
    }
    /** @brief Destroy the object and release its owned resources. */
    ~LogCapture() {
        std::clog.rdbuf(_previous);
        Ariadne::Logger::instance().configuration().set_verbosity(_verbosity);
        Ariadne::Logger::instance().configuration().set_theme(_theme);
    }
    /** @brief Disallow this construction to preserve the ownership contract. */
    LogCapture(const LogCapture&) = delete;
    /** @brief Disallow assignment to preserve the ownership contract. */
    LogCapture& operator=(const LogCapture&) = delete;
    /** @brief Return the configured diagnostic verbosity. */
    void verbosity(unsigned value) {
        auto& logger = Ariadne::Logger::instance();
        logger.configuration().set_verbosity(value == 0u ? 0u : logger.current_level() + value);
    }
    /** @brief Return the string representation. */
    std::string str() const { return _output.str(); }
    /** @brief Remove all elements. */
    void clear() { _output.str({}); }

  private:
    unsigned _verbosity;
    Ariadne::TerminalTextTheme _theme;
    std::ostringstream _output;
    std::streambuf* _previous = nullptr;
};

#endif

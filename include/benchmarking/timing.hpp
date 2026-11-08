/***************************************************************************
 *            benchmarking/timing.hpp
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

/*! \file benchmarking/timing.hpp
 *  \brief Steady-clock timing utilities with explicit execution boundaries.
 */

#ifndef ARIADNE_BENCHMARKING_TIMING_HPP
#define ARIADNE_BENCHMARKING_TIMING_HPP

#include <chrono>
#include <functional>
#include <utility>

namespace Ariadne::Benchmarking {

//! Monotonic clock used to measure elapsed wall time.
using Clock = std::chrono::steady_clock;
//! Floating-point elapsed time measured in nanoseconds.
using Duration = std::chrono::duration<double, std::nano>;

// Measure just the callable, leaving construction and verification to the
// caller. Preserve exceptions; do not log or allocate inside this timed scope.
template<class F>
//! Measure one invocation in nanoseconds, propagating callback exceptions.
double measure_ns(F&& operation) {
    auto const start = Clock::now();
    std::invoke(std::forward<F>(operation));
    auto const end = Clock::now();
    return Duration(end - start).count();
}

template<class F>
//! Measure one invocation in milliseconds.
double measure_ms(F&& operation) {
    return measure_ns(std::forward<F>(operation)) / 1e6;
}

//! Return the elapsed nanoseconds between two Clock timestamps.
inline double elapsed_ns(Clock::time_point start, Clock::time_point stop) {
    return Duration(stop - start).count();
}
} // namespace Ariadne::Benchmarking

#endif

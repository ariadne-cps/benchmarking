/***************************************************************************
 *            benchmarking/calibration.hpp
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

/*! \file benchmarking/calibration.hpp
 *  \brief Derive repetition counts from explicit pilot measurements.
 */

#ifndef ARIADNE_BENCHMARKING_CALIBRATION_HPP
#define ARIADNE_BENCHMARKING_CALIBRATION_HPP

#include "benchmarking/timing.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace Ariadne::Benchmarking {

// Choose a repetition count from explicitly supplied pilot timing. Do not
// silently run the measured operation, which might mutate its input.
//! Calculate repetitions required to reach a minimum total timed duration.
//! @param pilot_ns Duration of pilot_repetitions operations, in nanoseconds.
//! @param minimum_ns Desired minimum run time in nanoseconds.
//! @param pilot_repetitions Operation count represented by the pilot.
//! @param maximum_repetitions Maximum permitted count.
//! @return Positive repetition count capped by the maximum.
//! @throws std::invalid_argument for invalid or nonpositive parameters.
//! @throws std::overflow_error for nonfinite results.
//! @note Never invokes the operation, since it may mutate its inputs.
inline unsigned calibrated_repetitions(double pilot_ns,
        double minimum_ns, unsigned pilot_repetitions,
        unsigned maximum_repetitions) {
    if (not std::isfinite(pilot_ns) or pilot_ns <= 0.0
        or not std::isfinite(minimum_ns) or minimum_ns <= 0.0
        or pilot_repetitions == 0u or maximum_repetitions == 0u)
        throw std::invalid_argument("invalid calibration parameters");
    double const needed = std::ceil(
        minimum_ns / pilot_ns * static_cast<double>(pilot_repetitions));
    if (not std::isfinite(needed))
        throw std::overflow_error("calibration overflow");
    if (needed >= static_cast<double>(maximum_repetitions))
        return maximum_repetitions;
    return static_cast<unsigned>(needed < 1.0 ? 1.0 : needed);
}
} // namespace Ariadne::Benchmarking

#endif

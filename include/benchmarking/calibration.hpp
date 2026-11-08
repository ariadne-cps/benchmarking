#ifndef ARIADNE_BENCHMARKING_CALIBRATION_HPP
#define ARIADNE_BENCHMARKING_CALIBRATION_HPP

#include "benchmarking/timing.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace Ariadne::Benchmarking {

// Choose a repetition count from explicitly supplied pilot timing. Do not
// silently run the measured operation, which might mutate its input.
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

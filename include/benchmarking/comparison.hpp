/***************************************************************************
 *            benchmarking/comparison.hpp
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

/*! \file benchmarking/comparison.hpp
 *  \brief Ratio computations for cross-implementation performance.
 */

#ifndef ARIADNE_BENCHMARKING_COMPARISON_HPP
#define ARIADNE_BENCHMARKING_COMPARISON_HPP

#include <cmath>
#include <stdexcept>
#include <vector>

namespace Ariadne::Benchmarking {

// Baseline/candidate > 1 means candidate is faster.
//! Return baseline/candidate; values above one mean the candidate is faster.
//! @throws std::invalid_argument for nonpositive or nonfinite timings.
inline double speedup(double baseline, double candidate) {
    if (not std::isfinite(baseline) or not std::isfinite(candidate) or baseline <= 0.0 or candidate <= 0.0)
        throw std::invalid_argument("invalid timing ratio");
    return baseline / candidate;
}

//! Compute a geometric mean of positive finite ratios.
//! @throws std::invalid_argument for empty or invalid input.
inline double geometric_mean(const std::vector<double>& ratios) {
    if (ratios.empty())
        throw std::invalid_argument("empty ratio collection");
    double sum = 0.0;
    for (double r: ratios) {
        if (not std::isfinite(r) or r <= 0.0)
            throw std::invalid_argument("invalid ratio");
        sum += std::log(r);
    }
    return std::exp(sum / static_cast<double>(ratios.size()));
}
} // namespace Ariadne::Benchmarking

#endif

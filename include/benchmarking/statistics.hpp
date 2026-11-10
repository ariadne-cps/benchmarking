/***************************************************************************
 *            benchmarking/statistics.hpp
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

/*! \file benchmarking/statistics.hpp
 *  \brief Descriptive statistics for measured benchmark samples.
 */

#ifndef ARIADNE_BENCHMARKING_STATISTICS_HPP
#define ARIADNE_BENCHMARKING_STATISTICS_HPP

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "benchmarking/samples.hpp"

namespace Ariadne::Benchmarking {

//! Store minimum, maximum, median, mean, median absolute deviation (MAD),
//! nearest-rank p95, count and n-1 sample variance and standard deviation.
//! Timing statistics are nanoseconds; variance is nanoseconds squared.
struct Statistics {
    double median;
    double mean;
    double minimum;
    double maximum;
    double mad;
    double p95;
    std::size_t count;
    // Sample variance and standard deviation (n-1 denominator).
    // Undefined for n=1; represented by quiet NaN.
    //! Unbiased sample variance (n-1 denominator), NaN for one sample.
    double variance;
    //! Sample standard deviation, NaN for one sample.
    double stddev;
};

//! Find the median of nonempty sorted observations, averaging the middle two if even.
inline double median_of_sorted(const std::vector<double>& sorted) {
    if (sorted.empty())
        throw std::invalid_argument("empty sample collection");
    const std::size_t m = sorted.size() / 2u;
    return sorted.size() % 2u ? sorted[m]
                              : sorted[m - 1u] / 2.0 + sorted[m] / 2.0;
}

//! Calculate descriptive statistics using Welford's variance recurrence.
//! @throws std::invalid_argument if there are no samples.
inline Statistics summarize(const Samples& samples) {
    if (samples.size() == 0u)
        throw std::invalid_argument("no samples");
    auto sorted = samples.values();
    std::sort(sorted.begin(), sorted.end());
    const double med = median_of_sorted(sorted);
    std::vector<double> deviations;
    deviations.reserve(sorted.size());
    for (double value: sorted)
        deviations.push_back(std::abs(value - med));
    std::sort(deviations.begin(), deviations.end());
    const auto p95index = static_cast<std::size_t>(
                              std::ceil(0.95 * static_cast<double>(sorted.size()))) -
                          1u;
    // Welford's update avoids cancellation in E[X^2] - E[X]^2.
    double mean = 0.0;
    double squared_deviations = 0.0;
    std::size_t n = 0u;
    for (double value: samples.values()) {
        ++n;
        const double delta = value - mean;
        mean += delta / static_cast<double>(n);
        squared_deviations += delta * (value - mean);
    }
    const double variance = n > 1u
                              ? squared_deviations / static_cast<double>(n - 1u)
                              : std::numeric_limits<double>::quiet_NaN();
    return {med, mean, sorted.front(), sorted.back(),
            median_of_sorted(deviations), sorted[p95index], n,
            variance, std::sqrt(variance)};
}
} // namespace Ariadne::Benchmarking

#endif

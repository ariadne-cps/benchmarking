#ifndef ARIADNE_BENCHMARKING_STATISTICS_HPP
#define ARIADNE_BENCHMARKING_STATISTICS_HPP

#include "benchmarking/samples.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace Ariadne::Benchmarking {

struct Statistics {
    double median;
    double mean;
    double minimum;
    double maximum;
    double mad;
    double p95;
    std::size_t count;
};

inline double median_of_sorted(std::vector<double> const& sorted) {
    if (sorted.empty()) throw std::invalid_argument("empty sample collection");
    std::size_t const m = sorted.size() / 2u;
    return sorted.size() % 2u ? sorted[m]
        : sorted[m-1u]/2.0 + sorted[m]/2.0;
}

inline Statistics summarize(Samples const& samples) {
    if (samples.size() == 0u) throw std::invalid_argument("no samples");
    auto sorted = samples.values();
    std::sort(sorted.begin(), sorted.end());
    double const med = median_of_sorted(sorted);
    std::vector<double> deviations;
    deviations.reserve(sorted.size());
    for (double value : sorted) deviations.push_back(std::abs(value - med));
    std::sort(deviations.begin(), deviations.end());
    auto const p95index = static_cast<std::size_t>(
        std::ceil(0.95 * static_cast<double>(sorted.size()))) - 1u;
    return {med, std::accumulate(sorted.begin(), sorted.end(), 0.0)
                 / static_cast<double>(sorted.size()),
            sorted.front(), sorted.back(), median_of_sorted(deviations),
            sorted[p95index], sorted.size()};
}
} // namespace Ariadne::Benchmarking

#endif

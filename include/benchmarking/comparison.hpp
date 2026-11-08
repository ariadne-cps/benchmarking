#ifndef ARIADNE_BENCHMARKING_COMPARISON_HPP
#define ARIADNE_BENCHMARKING_COMPARISON_HPP

#include <cmath>
#include <stdexcept>
#include <vector>

namespace Ariadne::Benchmarking {

// Baseline/candidate > 1 means candidate is faster.
inline double speedup(double baseline, double candidate) {
    if (not std::isfinite(baseline) or not std::isfinite(candidate)
        or baseline <= 0.0 or candidate <= 0.0)
        throw std::invalid_argument("invalid timing ratio");
    return baseline / candidate;
}

inline double geometric_mean(std::vector<double> const& ratios) {
    if (ratios.empty()) throw std::invalid_argument("empty ratio collection");
    double sum = 0.0;
    for (double r : ratios) {
        if (not std::isfinite(r) or r <= 0.0)
            throw std::invalid_argument("invalid ratio");
        sum += std::log(r);
    }
    return std::exp(sum / static_cast<double>(ratios.size()));
}
} // namespace Ariadne::Benchmarking

#endif

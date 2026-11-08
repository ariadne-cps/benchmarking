#ifndef ARIADNE_BENCHMARKING_TIMING_HPP
#define ARIADNE_BENCHMARKING_TIMING_HPP

#include <chrono>
#include <functional>
#include <utility>

namespace Ariadne::Benchmarking {

using Clock = std::chrono::steady_clock;
using Duration = std::chrono::duration<double, std::nano>;

// Measure just the callable, leaving construction and verification to the
// caller. Preserve exceptions; do not log or allocate inside this timed scope.
template<class F>
double measure_ns(F&& operation) {
    auto const start = Clock::now();
    std::invoke(std::forward<F>(operation));
    auto const end = Clock::now();
    return Duration(end - start).count();
}

template<class F>
double measure_ms(F&& operation) {
    return measure_ns(std::forward<F>(operation)) / 1e6;
}

inline double elapsed_ns(Clock::time_point start, Clock::time_point stop) {
    return Duration(stop - start).count();
}
} // namespace Ariadne::Benchmarking

#endif

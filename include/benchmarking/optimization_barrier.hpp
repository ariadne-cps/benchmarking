#ifndef ARIADNE_BENCHMARKING_OPTIMIZATION_BARRIER_HPP
#define ARIADNE_BENCHMARKING_OPTIMIZATION_BARRIER_HPP

#include <atomic>

namespace Ariadne::Benchmarking {

// Keep a result observable without emitting output inside timed loops.
template<class T>
void keep_alive(T const& value) {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "g"(&value) : "memory");
#else
    static void const* volatile sink = nullptr;
    sink = &value;
    std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
}
} // namespace Ariadne::Benchmarking

#endif

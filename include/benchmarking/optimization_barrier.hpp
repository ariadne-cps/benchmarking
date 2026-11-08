/***************************************************************************
 *            benchmarking/optimization_barrier.hpp
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

/*! \file benchmarking/optimization_barrier.hpp
 *  \brief Keep timed computation results visible to the compiler.
 */

#ifndef ARIADNE_BENCHMARKING_OPTIMIZATION_BARRIER_HPP
#define ARIADNE_BENCHMARKING_OPTIMIZATION_BARRIER_HPP

#include <atomic>

namespace Ariadne::Benchmarking {

// Keep a result observable without emitting output inside timed loops.
template<class T>
//! Keep a computed value observable without printing during the measurement.
//! @note An optimization barrier is not a correctness assertion or CPU synchronization primitive.
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

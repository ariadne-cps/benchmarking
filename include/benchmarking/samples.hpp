/***************************************************************************
 *            benchmarking/samples.hpp
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

/*! \file benchmarking/samples.hpp
 *  \brief Finite, nonnegative elapsed-time observations.
 */

#ifndef ARIADNE_BENCHMARKING_SAMPLES_HPP
#define ARIADNE_BENCHMARKING_SAMPLES_HPP

#include <cmath>
#include <stdexcept>
#include <vector>

namespace Ariadne::Benchmarking {

//! Own an ordered collection of measured or normalized nanosecond timings.
class Samples {
  public:
    //! Append a finite, nonnegative duration in nanoseconds.
    //! @throws std::invalid_argument for negative or nonfinite values.
    void add(double elapsed) {
        if (not std::isfinite(elapsed) or elapsed < 0.0)
            throw std::invalid_argument("invalid elapsed time");
        _values.push_back(elapsed);
    }
    //! Return stored timing observations in insertion order.
    const std::vector<double>& values() const { return _values; }
    //! Return the number of recorded observations.
    std::size_t size() const { return _values.size(); }

  private:
    std::vector<double> _values;
};
} // namespace Ariadne::Benchmarking

#endif

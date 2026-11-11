/***************************************************************************
 *            benchmarking/logging.hpp
 *
 *  Copyright  2026  Luca Geretti
 *
 ****************************************************************************/

/*
 *  This file is part of Ariadne Benchmarking.
 *
 *  Ariadne Benchmarking is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Ariadne Benchmarking is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne Benchmarking. If not, see <https://www.gnu.org/licenses/>.
 */

/** @file benchmarking/logging.hpp
 *  @brief Initialize benchmark logging without requiring a thread manager.
 */

#ifndef ARIADNE_BENCHMARKING_LOGGING_HPP
#define ARIADNE_BENCHMARKING_LOGGING_HPP

#include "logging/logging.hpp"

namespace Ariadne::Benchmarking {

//! Initialize logger verbosity without attaching any thread registry.
inline void initialize_logging() {
    Logger::instance().configuration().set_verbosity(1u);
}
} // namespace Ariadne::Benchmarking

#endif

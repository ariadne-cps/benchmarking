/***************************************************************************
 *            benchmarking/logging.hpp
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

/*! \file benchmarking/logging.hpp
 *  \brief Initialize benchmark logging against the actual threading registry.
 */

#ifndef ARIADNE_BENCHMARKING_LOGGING_HPP
#define ARIADNE_BENCHMARKING_LOGGING_HPP

#include "logging/logging.hpp"
#include "threading/thread_manager.hpp"

namespace Ariadne::Benchmarking {

// ThreadManager owns the real Logger thread registry; never attach a
// synthetic single-thread registry that could conceal active workers.
//! Initialize ThreadManager's logger registry at default verbosity one.
//! Never replace the real worker registry with an artificial single-thread registry.
inline void initialize_logging() {
    ThreadManager::instance();
    Logger::instance().configuration().set_verbosity(1u);
}
} // namespace Ariadne::Benchmarking

#endif

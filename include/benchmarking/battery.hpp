/***************************************************************************
 *            benchmarking/battery.hpp
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

/*! \file benchmarking/battery.hpp
 *  \brief Type-safe benchmark battery for competing implementation types.
 */

#ifndef ARIADNE_BENCHMARKING_BATTERY_HPP
#define ARIADNE_BENCHMARKING_BATTERY_HPP

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#include "benchmarking/comparison_results.hpp"
#include "benchmarking/runner.hpp"

namespace Ariadne::Benchmarking {

// Each Implementation is a concrete C++ type. The workload's templated
// call operator is instantiated once per type; measured operations themselves
// are never invoked through type erasure or virtual dispatch.
//
// Workload contract:
//   workload.template operator()<Implementation>(record);
// where record(name, prepare, operation, verify, normalization=1.0) registers
// a distinct operation. All callbacks execute synchronously while the workload
// and its captured backend pool are alive.
template<class... Implementations>
//! Execute the same templated workload separately for each concrete implementation.
//! The record callback registers prepare, operation, verify and normalization for named operations.
class ComparisonBattery {
    static_assert(sizeof...(Implementations) > 0u,
                  "a battery needs at least one implementation");

  public:
    //! Register unique names for the implementation types.
    explicit ComparisonBattery(std::array<std::string, sizeof...(Implementations)> names)
        : _names(std::move(names)) {
        for (const auto& name: _names)
            _results.register_implementation(name);
    }

    //! Return accumulated measurements and comparison-ready statistics.
    const ComparisonResults& results() const { return _results; }

    template<class Workload>
    //! Run registered operations for each implementation at a given configuration.
    //! @param samples Count of measured rounds, excluding warm-ups.
    void run(const Configuration& configuration, const Runner& runner,
             unsigned samples, Workload&& workload) {
        const ComparisonContext context = ComparisonContext::from(configuration);
        runner.configuration_started(configuration);
        std::size_t index = 0u;
        // Workload is invoked separately for each concrete implementation.
        (run_one<Implementations>(_names[index++], context,
                                  runner, samples, configuration, workload),
         ...);
        runner.configuration_completed(configuration);
    }

    template<class Workload>
    //! Execute the workload for each configuration in a lazy parameter matrix.
    void run(const Matrix& matrix, const Runner& runner,
             unsigned samples, Workload&& workload) {
        matrix.for_each([&](const Configuration& configuration) {
            run(configuration, runner, samples, workload);
        });
    }

  private:
    template<class Implementation, class Workload>
    void run_one(const std::string& name, const ComparisonContext& context,
                 const Runner& runner, unsigned samples,
                 const Configuration& configuration, Workload& workload) {
        bool registered = false;
        auto record = [&](std::string operation_name,
                          auto&& prepare, auto&& operation, auto&& verify,
                          double normalization = 1.0) {
            Samples values = runner.run(operation_name, samples,
                                        std::forward<decltype(prepare)>(prepare),
                                        std::forward<decltype(operation)>(operation),
                                        std::forward<decltype(verify)>(verify),
                                        normalization);
            _results.add(name, std::move(operation_name), context, values);
            registered = true;
        };
        // Configuration-aware workloads are preferred. A one-argument
        // workload remains useful for fixed-input microbenchmarks.
        if constexpr (requires {
                          workload.template operator()<Implementation>(configuration, record);
                      }) {
            workload.template operator()<Implementation>(configuration, record);
        } else {
            workload.template operator()<Implementation>(record);
        }
        if (not registered)
            throw std::invalid_argument("implementation has no registered operations");
    }

    std::array<std::string, sizeof...(Implementations)> _names;
    ComparisonResults _results;
};

} // namespace Ariadne::Benchmarking

#endif

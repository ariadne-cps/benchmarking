#ifndef ARIADNE_BENCHMARKING_BATTERY_HPP
#define ARIADNE_BENCHMARKING_BATTERY_HPP

#include "benchmarking/comparison_results.hpp"
#include "benchmarking/runner.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

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
class ComparisonBattery {
    static_assert(sizeof...(Implementations) > 0u,
                  "a battery needs at least one implementation");
public:
    explicit ComparisonBattery(std::array<std::string, sizeof...(Implementations)> names)
        : _names(std::move(names)) {
        for (auto const& name : _names)
            _results.register_implementation(name);
    }

    ComparisonResults const& results() const { return _results; }

    template<class Workload>
    void run(Configuration const& configuration, Runner const& runner,
             unsigned samples, Workload&& workload) {
        ComparisonContext const context = ComparisonContext::from(configuration);
        runner.configuration_started(configuration);
        std::size_t index = 0u;
        // Workload is invoked separately for each concrete implementation.
        (run_one<Implementations>(_names[index++], context,
                                  runner, samples, configuration, workload), ...);
        runner.configuration_completed(configuration);
    }

    template<class Workload>
    void run(Matrix const& matrix, Runner const& runner,
             unsigned samples, Workload&& workload) {
        matrix.for_each([&](Configuration const& configuration) {
            run(configuration, runner, samples, workload);
        });
    }

private:
    template<class Implementation, class Workload>
    void run_one(std::string const& name, ComparisonContext const& context,
                 Runner const& runner, unsigned samples,
                 Configuration const& configuration, Workload& workload) {
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

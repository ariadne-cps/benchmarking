#ifndef ARIADNE_BENCHMARKING_LOGGING_HPP
#define ARIADNE_BENCHMARKING_LOGGING_HPP

#include "logging/logging.hpp"
#include "logging/thread_registry_interface.hpp"

namespace Ariadne::Benchmarking {

// Single-threaded fallback only. Threading-owned benchmarks must initialize
// ThreadManager first, so its real thread registry is already attached.
class SingleThreadRegistry final : public ThreadRegistryInterface {
public:
    Bool has_threads_registered() const override { return false; }
};

inline void initialize_logging() {
    static SingleThreadRegistry registry;
    Logger& logger = Logger::instance();
    if (not logger.has_thread_registry_attached()) {
        logger.attach_thread_registry(&registry);
    }
    logger.configuration().set_verbosity(1u);
}
} // namespace Ariadne::Benchmarking

#endif

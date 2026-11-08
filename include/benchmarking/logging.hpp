#ifndef ARIADNE_BENCHMARKING_LOGGING_HPP
#define ARIADNE_BENCHMARKING_LOGGING_HPP

#include "logging/logging.hpp"
#include "threading/thread_manager.hpp"

namespace Ariadne::Benchmarking {

// ThreadManager owns the real Logger thread registry; never attach a
// synthetic single-thread registry that could conceal active workers.
inline void initialize_logging() {
    ThreadManager::instance();
    Logger::instance().configuration().set_verbosity(1u);
}
} // namespace Ariadne::Benchmarking

#endif

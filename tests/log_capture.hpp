#ifndef ARIADNE_BENCHMARKING_TEST_LOG_CAPTURE_HPP
#define ARIADNE_BENCHMARKING_TEST_LOG_CAPTURE_HPP

#include "benchmarking/logging.hpp"
#include <iostream>
#include <sstream>

// These tests own the process logger and run without a ThreadManager. Capture
// its immediate output and restore formatting and the stream even on failure.
class LogCapture {
public:
    LogCapture()
        : _verbosity(Ariadne::Logger::instance().configuration().verbosity()),
          _theme(Ariadne::Logger::instance().configuration().theme()) {
        Ariadne::Logger::instance().use_immediate_scheduler();
        Ariadne::Logger::instance().configuration().set_theme(Ariadne::TT_THEME_NONE);
        _previous = std::clog.rdbuf(_output.rdbuf());
    }
    ~LogCapture() {
        std::clog.rdbuf(_previous);
        Ariadne::Logger::instance().configuration().set_verbosity(_verbosity);
        Ariadne::Logger::instance().configuration().set_theme(_theme);
    }
    LogCapture(LogCapture const&) = delete;
    LogCapture& operator=(LogCapture const&) = delete;
    void verbosity(unsigned value) {
        auto& logger = Ariadne::Logger::instance();
        logger.configuration().set_verbosity(value == 0u ? 0u : logger.current_level() + value);
    }
    std::string str() const { return _output.str(); }
    void clear() { _output.str({}); }
private:
    unsigned _verbosity;
    Ariadne::TerminalTextTheme _theme;
    std::ostringstream _output;
    std::streambuf* _previous = nullptr;
};

#endif

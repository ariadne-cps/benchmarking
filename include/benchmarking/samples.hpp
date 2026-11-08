#ifndef ARIADNE_BENCHMARKING_SAMPLES_HPP
#define ARIADNE_BENCHMARKING_SAMPLES_HPP

#include <cmath>
#include <stdexcept>
#include <vector>

namespace Ariadne::Benchmarking {

class Samples {
public:
    void add(double elapsed) {
        if (not std::isfinite(elapsed) or elapsed < 0.0)
            throw std::invalid_argument("invalid elapsed time");
        _values.push_back(elapsed);
    }
    std::vector<double> const& values() const { return _values; }
    std::size_t size() const { return _values.size(); }
private:
    std::vector<double> _values;
};
} // namespace Ariadne::Benchmarking

#endif

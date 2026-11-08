#ifndef ARIADNE_BENCHMARKING_PARAMETERS_HPP
#define ARIADNE_BENCHMARKING_PARAMETERS_HPP

#include "utility/command_line_interface.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Ariadne::Benchmarking {

namespace detail {

inline std::string trim(std::string const& value) {
    auto const is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    auto first = std::find_if_not(value.begin(), value.end(), is_space);
    auto last = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
    return first >= last ? std::string() : std::string(first, last);
}

// CSV with quoted items and backslash escapes (e.g. "a,b",c or a\\,b,c).
// Invalid or empty unquoted items are rejected instead of being skipped.
inline std::vector<std::string> split_values(std::string const& input) {
    std::vector<std::string> tokens;
    std::string token;
    bool quoted = false;
    bool escaped = false;
    bool had_quotes = false;
    for (char ch : input) {
        if (escaped) {
            token += ch;
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else if (ch == '"') {
            quoted = not quoted;
            had_quotes = true;
        } else if (ch == ',' and not quoted) {
            std::string value = trim(token);
            if (value.empty() and not had_quotes)
                throw std::invalid_argument("empty parameter value");
            tokens.push_back(std::move(value));
            token.clear();
            had_quotes = false;
        } else {
            token += ch;
        }
    }
    if (escaped or quoted)
        throw std::invalid_argument("unterminated escape or quotation");
    std::string value = trim(token);
    if (value.empty() and not had_quotes)
        throw std::invalid_argument("empty parameter value");
    tokens.push_back(std::move(value));
    return tokens;
}

template<class T>
T parse_value(std::string const& text) {
    if constexpr (std::is_same_v<T,std::string>) {
        return text;
    } else {
        if constexpr (std::is_integral_v<T> and std::is_unsigned_v<T>) {
            if (text.empty() or text[0] == '-')
                throw std::invalid_argument("negative unsigned parameter");
        }
        std::istringstream stream(text);
        T result{};
        if (not (stream >> result))
            throw std::invalid_argument("invalid parameter value");
        stream >> std::ws;
        if (not stream.eof())
            throw std::invalid_argument("trailing parameter text");
        return result;
    }
}

template<class T>
std::string format_value(T const& value) {
    std::ostringstream out;
    out << std::boolalpha << value;
    return out.str();
}

inline std::string quote_if_needed(std::string const& text) {
    if (text.find(',') == std::string::npos &&
        std::none_of(text.begin(), text.end(),
                     [](unsigned char c) { return std::isspace(c) != 0; }))
        return text;
    std::ostringstream out;
    out << std::quoted(text);
    return out.str();
}
} // namespace detail

class ParameterBase {
public:
    explicit ParameterBase(std::string name, std::string description)
        : _name(std::move(name)), _description(std::move(description)) {
        if (_name.empty())
            throw std::invalid_argument("parameter name must not be empty");
    }
    virtual ~ParameterBase() = default;
    ParameterBase(ParameterBase const&) = delete;
    ParameterBase& operator=(ParameterBase const&) = delete;

    std::string const& name() const { return _name; }
    std::string const& description() const { return _description; }
    virtual std::size_t size() const = 0;
    virtual std::string display_at(std::size_t index) const = 0;
    virtual ArgumentParser make_parser() = 0;

private:
    std::string _name;
    std::string _description;
};

// Every extra benchmark parameter accepts a scalar OR an ordered list.
// The parameter object must outlive the CLI acquisition and matrix iteration.
template<class T>
class Parameter final : public ParameterBase {
public:
    using Parser = std::function<T(std::string const&)>;
    using Formatter = std::function<std::string(T const&)>;

    Parameter(std::string name, std::string description, std::vector<T> defaults,
              Parser parser = detail::parse_value<T>,
              Formatter formatter = detail::format_value<T>)
        : ParameterBase(std::move(name), std::move(description)),
          _values(std::move(defaults)), _parse(std::move(parser)),
          _format(std::move(formatter)) {
        if (_values.empty() or not _parse or not _format)
            throw std::invalid_argument("parameter requires values and converters");
    }

    std::size_t size() const override { return _values.size(); }
    T const& at(std::size_t index) const { return _values.at(index); }
    std::vector<T> const& values() const { return _values; }
    std::string display_at(std::size_t index) const override {
        return detail::quote_if_needed(_format(at(index)));
    }

    ArgumentParser make_parser() override {
        return make_valued_argument_parser(
            "", String(name()),
            String(description() + " (single value or comma-separated list)"),
            [this](String const& value) -> VoidFunction {
                std::vector<T> parsed;
                for (auto const& token : detail::split_values(std::string(value)))
                    parsed.push_back(_parse(token));
                // Validate every element before returning the commit action.
                return [this, parsed = std::move(parsed)]() mutable {
                    _values = std::move(parsed);
                };
            });
    }

private:
    std::vector<T> _values;
    Parser _parse;
    Formatter _format;
};

class Configuration {
public:
    Configuration(std::vector<ParameterBase const*> axes,
                  std::vector<std::size_t> indices,
                  std::size_t ordinal, std::size_t count)
        : _axes(std::move(axes)), _indices(std::move(indices)),
          _ordinal(ordinal), _count(count) {}

    std::size_t ordinal() const { return _ordinal; }
    std::size_t count() const { return _count; }

    template<class T>
    T const& value(Parameter<T> const& axis) const {
        for (std::size_t i = 0; i < _axes.size(); ++i) {
            if (_axes[i] == &axis) return axis.at(_indices[i]);
        }
        throw std::invalid_argument("parameter does not belong to the matrix");
    }

    std::vector<std::pair<std::string, std::string>> dimensions() const {
        std::vector<std::pair<std::string, std::string>> result;
        result.reserve(_axes.size());
        for (std::size_t i = 0u; i < _axes.size(); ++i)
            result.emplace_back(_axes[i]->name(), _axes[i]->display_at(_indices[i]));
        return result;
    }

    std::string label() const {
        std::ostringstream out;
        for (std::size_t i = 0; i < _axes.size(); ++i) {
            if (i != 0u) out << ", ";
            out << _axes[i]->name() << '=' << _axes[i]->display_at(_indices[i]);
        }
        return out.str();
    }

private:
    std::vector<ParameterBase const*> _axes;
    std::vector<std::size_t> _indices;
    std::size_t _ordinal;
    std::size_t _count;
};

// Non-owning registration: the last axis varies fastest. Iteration is lazy,
// so even a large Cartesian product is never materialized in memory.
class Matrix {
public:
    Matrix& add(ParameterBase& axis) {
        for (auto const* present : _axes) {
            if (present == &axis or present->name() == axis.name())
                throw std::invalid_argument("duplicate benchmark parameter");
        }
        _axes.push_back(&axis);
        return *this;
    }

    std::size_t combinations() const {
        std::size_t count = 1u;
        for (auto const* axis : _axes) {
            if (axis->size() == 0u or
                count > std::numeric_limits<std::size_t>::max() / axis->size())
                throw std::overflow_error("benchmark matrix size overflow");
            count *= axis->size();
        }
        return count;
    }

    List<ArgumentParser> parsers() {
        List<ArgumentParser> result;
        for (auto* axis : _axes) result.append(axis->make_parser());
        return result;
    }

    std::string header() const {
        std::ostringstream out;
        out << "Parameters: ";
        if (_axes.empty()) out << "(none)";
        for (std::size_t i = 0; i < _axes.size(); ++i) {
            if (i != 0u) out << "; ";
            out << _axes[i]->name() << "=[";
            for (std::size_t j = 0; j < _axes[i]->size(); ++j) {
                if (j != 0u) out << ", ";
                out << _axes[i]->display_at(j);
            }
            out << ']';
        }
        out << " | configurations=" << combinations();
        return out.str();
    }

    template<class Function>
    void for_each(Function&& callback) const {
        std::size_t const total = combinations();
        std::vector<std::size_t> positions(_axes.size(), 0u);
        std::vector<ParameterBase const*> axes(_axes.begin(), _axes.end());
        for (std::size_t run = 0; run < total; ++run) {
            std::invoke(callback, Configuration(axes, positions, run + 1u, total));
            for (std::size_t i = positions.size(); i != 0u; --i) {
                if (++positions[i - 1u] < axes[i - 1u]->size()) break;
                positions[i - 1u] = 0u;
            }
        }
    }

private:
    std::vector<ParameterBase*> _axes;
};

} // namespace Ariadne::Benchmarking

#endif

/***************************************************************************
 *            benchmarking/parameters.hpp
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

/*! \file benchmarking/parameters.hpp
 *  \brief Typed command-line axes and lazy benchmark configuration matrices.
 */

#ifndef ARIADNE_BENCHMARKING_PARAMETERS_HPP
#define ARIADNE_BENCHMARKING_PARAMETERS_HPP

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

#include "utility/command_line_interface.hpp"

namespace Ariadne::Benchmarking {

namespace detail {

inline std::string trim(const std::string& value) {
    const auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    auto first = std::find_if_not(value.begin(), value.end(), is_space);
    auto last = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
    return first >= last ? std::string() : std::string(first, last);
}

// CSV with quoted items and backslash escapes (e.g. "a,b",c or a\\,b,c).
// Invalid or empty unquoted items are rejected instead of being skipped.
inline std::vector<std::string> split_values(const std::string& input) {
    std::vector<std::string> tokens;
    std::string token;
    bool quoted = false;
    bool escaped = false;
    bool had_quotes = false;
    for (char ch: input) {
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
T parse_value(const std::string& text) {
    if constexpr (std::is_same_v<T, std::string>) {
        return text;
    } else {
        if constexpr (std::is_integral_v<T> and std::is_unsigned_v<T>) {
            if (text.empty() or text[0] == '-')
                throw std::invalid_argument("negative unsigned parameter");
        }
        std::istringstream stream(text);
        T result{};
        if (not(stream >> result))
            throw std::invalid_argument("invalid parameter value");
        stream >> std::ws;
        if (not stream.eof())
            throw std::invalid_argument("trailing parameter text");
        return result;
    }
}

template<class T>
std::string format_value(const T& value) {
    std::ostringstream out;
    out << std::boolalpha << value;
    return out.str();
}

inline std::string quote_if_needed(const std::string& text) {
    if (text.find(',') == std::string::npos &&
        std::none_of(text.begin(), text.end(),
                     [](unsigned char c) { return std::isspace(c) != 0; }))
        return text;
    std::ostringstream out;
    out << std::quoted(text);
    return out.str();
}
} // namespace detail

//! Polymorphic non-owning interface for one named experiment axis.
class ParameterBase {
  public:
    //! Construct an axis with a name and command-line help description.
    explicit ParameterBase(std::string name, std::string description)
        : _name(std::move(name)), _description(std::move(description)) {
        if (_name.empty())
            throw std::invalid_argument("parameter name must not be empty");
    }
    virtual ~ParameterBase() = default;
    ParameterBase(const ParameterBase&) = delete;
    ParameterBase& operator=(const ParameterBase&) = delete;

    //! Return the axis name.
    const std::string& name() const { return _name; }
    //! Return the axis description shown in CLI help.
    const std::string& description() const { return _description; }
    //! Return the number of values on this axis.
    virtual std::size_t size() const = 0;
    //! Format one value for configuration labels.
    virtual std::string display_at(std::size_t index) const = 0;
    //! Create the argument parser for this axis.
    virtual ArgumentParser make_parser() = 0;

  private:
    std::string _name;
    std::string _description;
};

// Every extra benchmark parameter accepts a scalar OR an ordered list.
// The parameter object must outlive the CLI acquisition and matrix iteration.
template<class T>
//! Hold an ordered set of typed parameter values, including duplicates.
//! Accept a scalar or comma-separated sequence through the command line.
class Parameter final : public ParameterBase {
  public:
    using Parser = std::function<T(const std::string&)>;
    using Formatter = std::function<std::string(const T&)>;

    //! Construct a typed axis with nonempty defaults and optional conversion functions.
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
    //! Access the value at one index.
    const T& at(std::size_t index) const { return _values.at(index); }
    //! Return all configured values in order, with duplicates preserved.
    const std::vector<T>& values() const { return _values; }
    std::string display_at(std::size_t index) const override {
        return detail::quote_if_needed(_format(at(index)));
    }

    //! Parse and validate an entire replacement sequence before committing it.
    ArgumentParser make_parser() override {
        return make_valued_argument_parser(
            "", String(name()),
            String(description() + " (single value or comma-separated list)"),
            [this](const String& value) -> VoidFunction {
                std::vector<T> parsed;
                for (const auto& token: detail::split_values(std::string(value)))
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

//! One concrete combination of parameter positions with a one-based ordinal.
class Configuration {
  public:
    //! Capture axis references and selected indices for this configuration.
    Configuration(std::vector<const ParameterBase*> axes,
                  std::vector<std::size_t> indices,
                  std::size_t ordinal, std::size_t count)
        : _axes(std::move(axes)), _indices(std::move(indices)),
          _ordinal(ordinal), _count(count) {}

    //! Return this configuration's one-based ordinal.
    std::size_t ordinal() const { return _ordinal; }
    //! Return the number of configurations in the matrix.
    std::size_t count() const { return _count; }

    template<class T>
    //! Return the selected value of a registered typed parameter.
    //! @throws std::invalid_argument when the axis is absent.
    const T& value(const Parameter<T>& axis) const {
        for (std::size_t i = 0; i < _axes.size(); ++i) {
            if (_axes[i] == &axis)
                return axis.at(_indices[i]);
        }
        throw std::invalid_argument("parameter does not belong to the matrix");
    }

    //! Return named, formatted values in axis registration order.
    std::vector<std::pair<std::string, std::string>> dimensions() const {
        std::vector<std::pair<std::string, std::string>> result;
        result.reserve(_axes.size());
        for (std::size_t i = 0u; i < _axes.size(); ++i)
            result.emplace_back(_axes[i]->name(), _axes[i]->display_at(_indices[i]));
        return result;
    }

    //! Produce a human-readable label listing selected axis values.
    std::string label() const {
        std::ostringstream out;
        for (std::size_t i = 0; i < _axes.size(); ++i) {
            if (i != 0u)
                out << ", ";
            out << _axes[i]->name() << '=' << _axes[i]->display_at(_indices[i]);
        }
        return out.str();
    }

  private:
    std::vector<const ParameterBase*> _axes;
    std::vector<std::size_t> _indices;
    std::size_t _ordinal;
    std::size_t _count;
};

// Non-owning registration: the last axis varies fastest. Iteration is lazy,
// so even a large Cartesian product is never materialized in memory.
//! Lazily iterate a Cartesian product of non-owned typed parameter axes.
//! The last axis varies fastest; an empty matrix denotes one configuration.
class Matrix {
  public:
    //! Register a uniquely named live axis without taking ownership.
    Matrix& add(ParameterBase& axis) {
        for (const auto* present: _axes) {
            if (present == &axis or present->name() == axis.name())
                throw std::invalid_argument("duplicate benchmark parameter");
        }
        _axes.push_back(&axis);
        return *this;
    }

    //! Compute the number of cases without materializing them.
    //! @throws std::overflow_error when the Cartesian size overflows.
    std::size_t combinations() const {
        std::size_t count = 1u;
        for (const auto* axis: _axes) {
            if (axis->size() == 0u or
                count > std::numeric_limits<std::size_t>::max() / axis->size())
                throw std::overflow_error("benchmark matrix size overflow");
            count *= axis->size();
        }
        return count;
    }

    //! Return parsers used to expose matrix axes in CommandLine.
    List<ArgumentParser> parsers() {
        List<ArgumentParser> result;
        for (auto* axis: _axes)
            result.append(axis->make_parser());
        return result;
    }

    //! Describe every axis sequence and the total number of configurations.
    std::string header() const {
        std::ostringstream out;
        out << "Parameters: ";
        if (_axes.empty())
            out << "(none)";
        for (std::size_t i = 0; i < _axes.size(); ++i) {
            if (i != 0u)
                out << "; ";
            out << _axes[i]->name() << "=[";
            for (std::size_t j = 0; j < _axes[i]->size(); ++j) {
                if (j != 0u)
                    out << ", ";
                out << _axes[i]->display_at(j);
            }
            out << ']';
        }
        out << " | configurations=" << combinations();
        return out.str();
    }

    template<class Function>
    //! Visit each combination lazily, passing a temporary Configuration.
    void for_each(Function&& callback) const {
        const std::size_t total = combinations();
        std::vector<std::size_t> positions(_axes.size(), 0u);
        std::vector<const ParameterBase*> axes(_axes.begin(), _axes.end());
        for (std::size_t run = 0; run < total; ++run) {
            std::invoke(callback, Configuration(axes, positions, run + 1u, total));
            for (std::size_t i = positions.size(); i != 0u; --i) {
                if (++positions[i - 1u] < axes[i - 1u]->size())
                    break;
                positions[i - 1u] = 0u;
            }
        }
    }

  private:
    std::vector<ParameterBase*> _axes;
};

} // namespace Ariadne::Benchmarking

#endif

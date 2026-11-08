/***************************************************************************
 *            benchmarking/summary_fields.hpp
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

/*! \file benchmarking/summary_fields.hpp
 *  \brief Choose and parse benchmark summary-statistics columns.
 */

#ifndef ARIADNE_BENCHMARKING_SUMMARY_FIELDS_HPP
#define ARIADNE_BENCHMARKING_SUMMARY_FIELDS_HPP

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace Ariadne::Benchmarking {

//! Identifier for a supported summary statistic or sample count.
enum class SummaryField {
    Minimum, Maximum, Mean, MAD, Median, P95, Stddev, Variance, Count
};
//! Ordered set of selected summary fields.
using SummaryFields = std::vector<SummaryField>;

// Default columns are intentionally concise, and preserve the user's order.
//! Return the default min, max, mean and MAD columns.
inline SummaryFields default_summary_fields() {
    return {SummaryField::Minimum, SummaryField::Maximum,
            SummaryField::Mean, SummaryField::MAD};
}

//! Return the short identifier accepted by --summary.
inline std::string_view summary_field_id(SummaryField field) {
    switch (field) {
        case SummaryField::Minimum: return "min";
        case SummaryField::Maximum: return "max";
        case SummaryField::Mean: return "mean";
        case SummaryField::MAD: return "mad";
        case SummaryField::Median: return "median";
        case SummaryField::P95: return "p95";
        case SummaryField::Stddev: return "stddev";
        case SummaryField::Variance: return "variance";
        case SummaryField::Count: return "count";
        default: throw std::invalid_argument("unknown summary field");
    }
}

//! Return a display label including timing units where applicable.
inline std::string_view summary_field_heading(SummaryField field) {
    switch (field) {
        case SummaryField::Minimum: return "Min (ns)";
        case SummaryField::Maximum: return "Max (ns)";
        case SummaryField::Mean: return "Mean (ns)";
        case SummaryField::MAD: return "MAD (ns)";
        case SummaryField::Median: return "Median (ns)";
        case SummaryField::P95: return "P95 (ns)";
        case SummaryField::Stddev: return "Stddev (ns)";
        case SummaryField::Variance: return "Variance (ns^2)";
        case SummaryField::Count: return "N";
        default: throw std::invalid_argument("unknown summary field");
    }
}

// Values use the names accepted by --summary. Duplicates are rejected because
// they would create identical report columns, unlike duplicate experiment axes.
//! Parse comma-separated field names in order; reject empty, unknown or duplicate fields.
inline SummaryFields parse_summary_fields(std::string const& input) {
    SummaryFields result;
    std::size_t position = 0u;
    do {
        std::size_t const comma = input.find(',', position);
        std::string_view field(input.data() + position,
            (comma == std::string::npos ? input.size() : comma) - position);
        while (not field.empty() and
               std::isspace(static_cast<unsigned char>(field.front())) != 0)
            field.remove_prefix(1);
        while (not field.empty() and
               std::isspace(static_cast<unsigned char>(field.back())) != 0)
            field.remove_suffix(1);
        bool recognized = false;
        for (SummaryField candidate : {
                SummaryField::Minimum, SummaryField::Maximum,
                SummaryField::Mean, SummaryField::MAD,
                SummaryField::Median, SummaryField::P95,
                SummaryField::Stddev, SummaryField::Variance,
                SummaryField::Count}) {
            if (field == summary_field_id(candidate)) {
                if (std::find(result.begin(), result.end(), candidate) != result.end())
                    throw std::invalid_argument("duplicate --summary column: " +
                                                std::string(field));
                result.push_back(candidate);
                recognized = true;
                break;
            }
        }
        if (not recognized)
            throw std::invalid_argument("unknown or empty --summary column: " +
                                        std::string(field));
        if (comma == std::string::npos) break;
        position = comma + 1u;
    } while (true);
    return result;
}

} // namespace Ariadne::Benchmarking

#endif

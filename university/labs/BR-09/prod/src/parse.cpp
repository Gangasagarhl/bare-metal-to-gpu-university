// SPDX-License-Identifier: LicenseRef-Uni-Lab
#include "uni/parse.h"

#include <charconv>
#include <cmath>
#include <system_error>

namespace uni {

const char* describe(ParseError e)
{
    switch (e) {
    case ParseError::empty:
        return "is empty";
    case ParseError::not_a_number:
        return "is not a number";
    case ParseError::trailing_characters:
        return "has characters after the number";
    case ParseError::not_finite:
        return "is not a finite number";
    case ParseError::out_of_range:
        return "is outside the range of double";
    }
    return "is invalid";
}

std::variant<double, ParseError> parse_number(std::string_view text)
{
    if (text.empty()) {
        return ParseError::empty;
    }
    // from_chars accepts a leading '-' but not '+'; version 0.1 accepted "+3", keep it.
    if (text.front() == '+') {
        text.remove_prefix(1);
        if (text.empty() || text.front() == '-' || text.front() == '+') {
            return ParseError::not_a_number;
        }
    }
    double value = 0.0;
    const char* first = text.data();
    const char* last = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, value, std::chars_format::general);
    if (ec == std::errc::invalid_argument) {
        return ParseError::not_a_number;
    }
    if (ec == std::errc::result_out_of_range) {
        return ParseError::out_of_range;
    }
    if (ptr != last) {
        return ParseError::trailing_characters;
    }
    if (!std::isfinite(value)) {
        return ParseError::not_finite;  // "nan", "inf", "infinity"
    }
    return value;
}

}  // namespace uni

// SPDX-License-Identifier: LicenseRef-Uni-Lab
// uni::parse_number: strict parsing of one command-line number (new in 1.0.0).
// Accepted: an optional '+' or '-', decimal digits, an optional '.', an optional
// exponent (1e3), and nothing else. Rejected, with a reason: empty text, trailing
// characters ("3abc"), hexadecimal ("0x10"), spaces, NaN, infinity, and values
// outside the range of double ("1e999"). The decimal point is always '.', whatever
// the machine's locale (std::from_chars does not use the locale).
#pragma once

#include <string_view>
#include <variant>

namespace uni {

enum class ParseError
{
    empty,
    not_a_number,
    trailing_characters,
    not_finite,
    out_of_range,
};

// Short, stable text for logs and monitoring ("E-PARSE" style codes are in app/main.cpp).
const char* describe(ParseError e);

std::variant<double, ParseError> parse_number(std::string_view text);

}  // namespace uni

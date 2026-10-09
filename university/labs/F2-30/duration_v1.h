// duration_v1.h: the first attempt, written before the combined-unit tests existed.
// It accepts exactly one number followed by one unit.
#pragma once

#include <optional>
#include <string_view>

inline std::optional<long> parse_duration(std::string_view text)
{
    if (text.size() < 2) {
        return std::nullopt;
    }
    long number = 0;
    for (std::size_t i = 0; i + 1 < text.size(); ++i) {
        if (text[i] < '0' || text[i] > '9') {
            return std::nullopt;
        }
        number = number * 10 + (text[i] - '0');
    }
    switch (text.back()) {
    case 'h': return number * 3600;
    case 'm': return number * 60;
    case 's': return number;
    default: return std::nullopt;
    }
}

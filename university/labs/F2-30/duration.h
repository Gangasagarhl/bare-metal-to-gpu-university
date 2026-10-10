// duration.h: parse a duration such as "1h30m", "45s" or "2m5s" into seconds.
// Rules: one or more <number><unit> parts; units h, m, s, in that order, each at most once.
// Anything else (empty text, a number without a unit, an unknown unit, a repeated or
// out-of-order unit) gives no value.
#pragma once

#include <optional>
#include <string_view>

inline std::optional<long> parse_duration(std::string_view text)
{
    if (text.empty()) {
        return std::nullopt;
    }
    long total = 0;
    int last_rank = -1;                       // h = 0, m = 1, s = 2: units must go down
    std::size_t i = 0;
    while (i < text.size()) {
        long number = 0;
        std::size_t digits = 0;
        while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
            number = number * 10 + (text[i] - '0');
            ++i;
            ++digits;
        }
        if (digits == 0 || i == text.size()) {
            return std::nullopt;              // a unit without a number, or a number without a unit
        }
        int rank = 0;
        long scale = 0;
        switch (text[i]) {
        case 'h': rank = 0; scale = 3600; break;
        case 'm': rank = 1; scale = 60; break;
        case 's': rank = 2; scale = 1; break;
        default: return std::nullopt;         // unknown unit
        }
        if (rank <= last_rank) {
            return std::nullopt;              // repeated or out of order
        }
        last_rank = rank;
        total += number * scale;
        ++i;
    }
    return total;
}

// convert.cpp - F12-19 Listing 1: three ways to put a double into a 16-bit integer.
// The failure class: a value that "always fitted" in an old system no longer fits in a new
// one, and the conversion either wraps silently, saturates, or is detected and handled.
#include <cstdint>
#include <cstdio>
#include <limits>
#include <optional>
#include <string>

using Lim = std::numeric_limits<std::int16_t>;

// Wrapping: double -> int32 (in range for these values), then int32 -> int16.
// Since C++20 the second conversion is defined: the value is reduced modulo 2^16.
std::int16_t to_i16_wrapping(double v)
{
    return static_cast<std::int16_t>(static_cast<std::int32_t>(v));
}

// Saturating: clamp to the representable range, so the sign can never flip.
std::int16_t to_i16_saturating(double v)
{
    if (v >= Lim::max()) {
        return Lim::max();
    }
    if (v <= Lim::min()) {
        return Lim::min();
    }
    return static_cast<std::int16_t>(v);
}

// Checked: refuse values that do not fit; the caller must decide what that means.
std::optional<std::int16_t> to_i16_checked(double v)
{
    if (!(v >= Lim::min() && v <= Lim::max())) {  // also refuses NaN
        return std::nullopt;
    }
    return static_cast<std::int16_t>(v);
}

int main()
{
    std::printf("int16 range: %d .. %d\n\n", Lim::min(), Lim::max());
    std::printf("%10s  %10s  %10s  %s\n", "value", "wrapping", "saturating", "checked");
    for (const double v : {1000.0, 20000.0, 32767.0, 32768.0, 40000.0, 65535.0, 70000.0,
                           -40000.0}) {
        const auto c = to_i16_checked(v);
        std::printf("%10.1f  %10d  %10d  %s\n", v, to_i16_wrapping(v), to_i16_saturating(v),
                    c ? std::to_string(*c).c_str() : "out of range: handled by caller");
    }
    return 0;
}

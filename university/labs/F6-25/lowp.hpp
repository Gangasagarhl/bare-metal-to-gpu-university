// lowp.hpp: round a value to a narrower binary floating-point format (round to nearest,
// ties to even), the university's own model. p = significand bits including the hidden
// bit; emin = exponent of the smallest normal number. Overflow is not modelled: callers
// keep values inside the format's range. Checked against NVIDIA's conversions in formats.cu.
#pragma once
#include <cmath>

struct Format
{
    const char* name;
    int p;        // significand bits (fraction bits + 1)
    int emin;     // smallest normal is 2^emin
};

inline constexpr Format FP32{"FP32", 24, -126};
inline constexpr Format TF32{"TF32", 11, -126};
inline constexpr Format FP16{"FP16", 11, -14};
inline constexpr Format BF16{"BF16", 8, -126};
inline constexpr Format E4M3{"FP8 E4M3", 4, -6};
inline constexpr Format E5M2{"FP8 E5M2", 3, -14};

inline double roundTo(double x, const Format& f)
{
    if (x == 0.0 || !std::isfinite(x)) {
        return x;
    }
    int e = 0;
    std::frexp(x, &e);                 // |x| in [2^(e-1), 2^e)
    int exp = e - 1;
    if (exp < f.emin) {
        exp = f.emin;                  // subnormal range: the spacing stops shrinking
    }
    const double spacing = std::ldexp(1.0, exp - (f.p - 1));
    return std::nearbyint(x / spacing) * spacing;   // default mode: nearest, ties to even
}

inline double unitRoundoff(const Format& f)
{
    return std::ldexp(1.0, -f.p);      // u = 2^-p: the largest relative rounding error
}

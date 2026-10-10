// F0-80 shared code: the university's model of binary floating-point formats with fewer bits.
// A format is described by p (significand bits, including the implicit leading 1), emin and emax.
// roundTo(x, f) returns the value of f nearest to x (round to nearest, ties to even, IEEE-like:
// gradual underflow to subnormals, overflow to infinity). It is a model: see the chapter's notes on
// formats whose special values differ from the IEEE pattern.
#pragma once
#include <cmath>
#include <cstdint>
#include <limits>

struct Format
{
    const char* name;
    int p;    // significand precision in bits (includes the hidden bit)
    int emin; // exponent of the smallest normal number
    int emax; // exponent of the largest finite number
};

inline constexpr Format kBinary32{"binary32 (FP32)", 24, -126, 127};
inline constexpr Format kBinary16{"binary16 (FP16)", 11, -14, 15};
inline constexpr Format kBfloat16{"bfloat16 (BF16)", 8, -126, 127};
inline constexpr Format kE5M2{"FP8 E5M2 model", 3, -14, 15};
inline constexpr Format kE4M3{"FP8 E4M3 model", 4, -6, 7};

inline double unitRoundoff(const Format& f)
{
    return std::ldexp(1.0, -f.p);
}

inline double maxFinite(const Format& f)
{
    return (2.0 - std::ldexp(1.0, 1 - f.p)) * std::ldexp(1.0, f.emax);
}

inline double minNormal(const Format& f)
{
    return std::ldexp(1.0, f.emin);
}

inline double minSubnormal(const Format& f)
{
    return std::ldexp(1.0, f.emin - (f.p - 1));
}

inline double roundTo(double x, const Format& f)
{
    if (x == 0.0 || !std::isfinite(x)) {
        return x;
    }
    int e = std::ilogb(x); // |x| = m * 2^e with 1 <= m < 2
    if (e < f.emin) {
        e = f.emin; // subnormal range: fixed spacing 2^(emin - p + 1)
    }
    const double quantum = std::ldexp(1.0, e - (f.p - 1));
    const double r =
        std::nearbyint(x / quantum) * quantum; // default rounding mode: to nearest, ties to even
    if (std::fabs(r) > maxFinite(f)) {
        return std::copysign(std::numeric_limits<double>::infinity(), x);
    }
    return r;
}

// splitmix64, as in F0-75: reproducible data on every machine.
inline std::uint64_t splitmix64(std::uint64_t& state)
{
    std::uint64_t z = (state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

inline double uniformMinus1To1(std::uint64_t& state)
{
    return 2.0 * std::ldexp(static_cast<double>(splitmix64(state) >> 11), -53) - 1.0;
}

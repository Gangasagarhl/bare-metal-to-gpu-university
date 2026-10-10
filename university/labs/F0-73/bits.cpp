// F0-73 Listing 1: take IEEE 754 binary32 numbers (C++ float on this machine) apart, field by
// field.
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

// Print the three fields of a float and rebuild its value from them.
void show(const char* label, float f)
{
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(f);
    const unsigned sign = bits >> 31;
    const unsigned expField = (bits >> 23) & 0xFFu; // 8 bits
    const std::uint32_t frac = bits & 0x7FFFFFu;    // 23 bits
    const char* kind = "normal";
    double rebuilt = 0.0;
    if (expField == 0xFFu) {
        kind = (frac == 0) ? "infinity" : "NaN";
    } else if (expField == 0) {
        kind = (frac == 0) ? "zero" : "subnormal";
        rebuilt = std::ldexp(static_cast<double>(frac), -149); // 0.f x 2^-126
    } else { // 1.f x 2^(e-127), with the hidden 1 put back as bit 23
        rebuilt =
            std::ldexp(static_cast<double>(frac | 0x800000u), static_cast<int>(expField) - 150);
    }
    if (sign != 0) {
        rebuilt = -rebuilt;
    }
    std::printf("%-22s 0x%08X  s=%u e=%3u (2^%4d) f=0x%06X  %-9s", label,
                static_cast<unsigned>(bits), sign, expField,
                expField == 0 ? -126 : static_cast<int>(expField) - 127,
                static_cast<unsigned>(frac), kind);
    if (expField != 0xFFu) {
        std::printf("  value = %.30g", rebuilt);
    }
    std::printf("\n");
}

int main()
{
    show("1", 1.0f);
    show("-2.5", -2.5f);
    show("6.5", 6.5f);
    show("0.1", 0.1f);
    show("1/3", 1.0f / 3.0f);
    show("16777216 (2^24)", 16777216.0f);
    show("16777217 (2^24 + 1)", 16777217.0f);
    show("largest finite", std::numeric_limits<float>::max());
    show("smallest normal", std::numeric_limits<float>::min());
    show("smallest subnormal", std::numeric_limits<float>::denorm_min());
    show("+0", 0.0f);
    show("-0", -0.0f);
    show("+infinity", std::numeric_limits<float>::infinity());
    show("quiet NaN", std::numeric_limits<float>::quiet_NaN());
    return 0;
}

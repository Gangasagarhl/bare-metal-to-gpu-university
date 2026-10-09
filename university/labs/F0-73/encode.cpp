// F0-73 worked example check: encode 6.5 and -0.75 by hand, then compare with the machine.
#include <bit>
#include <cstdint>
#include <cstdio>

std::uint32_t byHand(unsigned sign, int unbiasedExp, std::uint32_t fraction23)
{
    return (static_cast<std::uint32_t>(sign) << 31) |
           (static_cast<std::uint32_t>(unbiasedExp + 127) << 23) | fraction23;
}

int main()
{
    // 6.5 = 110.1 (binary) = 1.101 x 2^2 -> sign 0, exponent 2, fraction 101000...0
    const std::uint32_t h1 = byHand(0, 2, 0b101u << 20);
    // -0.75 = -0.11 (binary) = -1.1 x 2^-1 -> sign 1, exponent -1, fraction 1000...0
    const std::uint32_t h2 = byHand(1, -1, 0b1u << 22);
    std::printf("6.5   by hand 0x%08X  machine 0x%08X  %s\n", static_cast<unsigned>(h1),
                static_cast<unsigned>(std::bit_cast<std::uint32_t>(6.5f)),
                h1 == std::bit_cast<std::uint32_t>(6.5f) ? "MATCH" : "DIFFER");
    std::printf("-0.75 by hand 0x%08X  machine 0x%08X  %s\n", static_cast<unsigned>(h2),
                static_cast<unsigned>(std::bit_cast<std::uint32_t>(-0.75f)),
                h2 == std::bit_cast<std::uint32_t>(-0.75f) ? "MATCH" : "DIFFER");
    return (h1 == std::bit_cast<std::uint32_t>(6.5f) && h2 == std::bit_cast<std::uint32_t>(-0.75f))
               ? 0
               : 1;
}

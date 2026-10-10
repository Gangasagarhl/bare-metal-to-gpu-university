// rgmii_model.h - F1-51: the university's toy model of a clock-forwarded, double-data-rate
// MAC-to-PHY link (the idea behind RGMII), used by the forensic lab.
// Units are the simulator's own "ticks"; no number here is a real RGMII timing value.
//   - the sender changes data every half clock period, exactly at clock edges;
//   - the receiver samples at each clock edge plus the total clock delay D
//     (delay added by the PHY + by the MAC + by the circuit-board trace);
//   - near a data change the line is unstable: a sample there reads a random bit.
#pragma once
#include <cstdint>
#include <vector>

namespace toy {

constexpr int kHalfPeriod = 4;     // ticks between data changes
constexpr int kUnstable = 1;       // samples within 1 tick of a change are unreliable

inline std::uint32_t rnd(std::uint32_t& s)
{
    s = s * 1103515245u + 12345u;
    return (s >> 16) & 0x7FFFu;
}

inline std::uint32_t crc32(const std::vector<std::uint8_t>& d)
{
    std::uint32_t c = 0xFFFFFFFFu;
    for (std::uint8_t b : d) {
        c ^= b;
        for (int i = 0; i < 8; ++i) {
            c = (c & 1u) ? (c >> 1) ^ 0xEDB88320u : c >> 1;
        }
    }
    return ~c;
}

// Distance from the sampling instant to the nearest data change, in ticks.
inline int margin(int totalDelay)
{
    const int pos = ((totalDelay % kHalfPeriod) + kHalfPeriod) % kHalfPeriod;
    return pos < kHalfPeriod - pos ? pos : kHalfPeriod - pos;
}

// Sends `frames` frames of 64 random bytes; returns how many arrived with a good CRC.
inline int receiveGood(int totalDelay, int frames, std::uint32_t seed)
{
    const bool unstable = margin(totalDelay) <= kUnstable;
    int good = 0;
    for (int f = 0; f < frames; ++f) {
        std::vector<std::uint8_t> sent(64);
        for (auto& b : sent) {
            b = static_cast<std::uint8_t>(rnd(seed));
        }
        const std::uint32_t fcs = crc32(sent);
        std::vector<std::uint8_t> got = sent;
        if (unstable) {
            for (auto& b : got) {
                for (int bit = 0; bit < 8; ++bit) {
                    if (rnd(seed) % 2 == 0) {        // half of the unstable samples flip
                        b = static_cast<std::uint8_t>(b ^ (1u << bit));
                    }
                }
            }
        }
        if (crc32(got) == fcs) {
            ++good;
        }
    }
    return good;
}

}  // namespace toy

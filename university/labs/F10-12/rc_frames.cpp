// F10-12 Listing 1: "U-RC", the university's teaching RC frame on a serial link.
// 1 start byte 0xA5, 8 channels of 11 bits packed into 11 bytes (least significant bit
// first), 1 flags byte (bit 0: receiver in failsafe), 1 CRC-8 byte (polynomial 0x07) over
// the 12 bytes after the start byte: 14 bytes in all. Channel value 0..2047 maps to a
// 1000..2000 us stick position. Every detail is the university's invention.
#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>

using Frame = std::array<std::uint8_t, 14>;

std::uint8_t crc8(const std::uint8_t* p, int n)
{
    std::uint8_t c = 0;
    for (int i = 0; i < n; ++i) {
        c ^= p[i];
        for (int b = 0; b < 8; ++b) {
            c = static_cast<std::uint8_t>((c & 0x80) ? (c << 1) ^ 0x07 : (c << 1));
        }
    }
    return c;
}

Frame pack(const std::array<std::uint16_t, 8>& ch, bool failsafe)
{
    Frame f{};
    f[0] = 0xA5;
    int bit = 0;
    for (std::uint16_t v : ch) {
        for (int k = 0; k < 11; ++k, ++bit) {
            if ((v >> k) & 1) {
                f[1 + bit / 8] = static_cast<std::uint8_t>(f[1 + bit / 8] | (1u << (bit % 8)));
            }
        }
    }
    f[12] = failsafe ? 1 : 0;
    f[13] = crc8(&f[1], 12);
    return f;
}

bool unpack(const Frame& f, std::array<std::uint16_t, 8>& ch, bool& failsafe)
{
    if (f[0] != 0xA5 || crc8(&f[1], 12) != f[13]) {
        return false;
    }
    int bit = 0;
    for (auto& v : ch) {
        v = 0;
        for (int k = 0; k < 11; ++k, ++bit) {
            if ((f[1 + bit / 8] >> (bit % 8)) & 1) {
                v = static_cast<std::uint16_t>(v | (1u << k));
            }
        }
    }
    failsafe = (f[12] & 1) != 0;
    return true;
}

double toUs(std::uint16_t v)
{
    return 1000.0 + v * 1000.0 / 2047.0;
}

int main()
{
    const std::array<std::uint16_t, 8> sticks{1024, 1024, 0, 1024, 2047, 0, 512, 1536};
    const Frame f = pack(sticks, false);
    std::printf("frame:");
    for (auto b : f) {
        std::printf(" %02X", b);
    }
    std::printf("\n");
    std::array<std::uint16_t, 8> ch{};
    bool fs = false;
    if (unpack(f, ch, fs)) {
        std::printf("decoded (us):");
        for (auto v : ch) {
            std::printf(" %.0f", toUs(v));
        }
        std::printf("  failsafe flag %d\n", fs ? 1 : 0);
    }
    Frame bad = f;
    bad[3] ^= 0x10;                       // one bit flipped on the wire
    std::printf("one flipped bit: %s\n", unpack(bad, ch, fs) ? "accepted" : "rejected by CRC");
    std::printf("bytes per frame %zu; at 100 frames/s that is %zu bytes/s of payload\n",
                f.size(), f.size() * 100);
    return 0;
}

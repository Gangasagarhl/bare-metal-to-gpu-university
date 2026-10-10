// F10-10 Listing 2: "U-Shot", the university's teaching version of a digital ESC frame.
// 16 bits, most significant first: 11-bit value, 1 telemetry-request bit, 4-bit checksum
// (XOR of the three 4-bit groups of the first 12 bits). Values 0..47 are reserved for
// commands, 48..2047 are throttle. Each bit is one pulse in a fixed bit period; a 1 is a
// long pulse, a 0 a short pulse. The bit rate and pulse ratios are exercise values.
// Whether this matches a real protocol is a question for the chapter's unverified box.
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <vector>

constexpr double kBitRate = 300000.0;   // bits per second (exercise value)
constexpr double kOneHigh = 0.70;       // fraction of the bit period a 1 stays high
constexpr double kZeroHigh = 0.35;      // fraction for a 0

std::uint16_t encode(std::uint16_t value, bool telemetry)
{
    const auto data = static_cast<std::uint16_t>((value << 1) | (telemetry ? 1 : 0));
    const auto crc = static_cast<std::uint16_t>((data ^ (data >> 4) ^ (data >> 8)) & 0xF);
    return static_cast<std::uint16_t>((data << 4) | crc);
}

bool decode(std::uint16_t frame, std::uint16_t& value, bool& telemetry)
{
    const auto data = static_cast<std::uint16_t>(frame >> 4);
    const auto crc = static_cast<std::uint16_t>((data ^ (data >> 4) ^ (data >> 8)) & 0xF);
    if (crc != (frame & 0xF)) {
        return false;                       // the ESC must ignore a frame that fails the check
    }
    value = static_cast<std::uint16_t>(data >> 1);
    telemetry = (data & 1) != 0;
    return true;
}

// The pulse widths (in ns) a timer would produce for one frame.
std::vector<int> pulses(std::uint16_t frame)
{
    const double bitNs = 1e9 / kBitRate;
    std::vector<int> w;
    for (int b = 15; b >= 0; --b) {
        w.push_back(static_cast<int>(bitNs * (((frame >> b) & 1) ? kOneHigh : kZeroHigh)));
    }
    return w;
}

int main()
{
    const double bitNs = 1e9 / kBitRate;
    std::printf("bit period %.1f ns, frame of 16 bits %.2f us\n", bitNs, 16 * bitNs / 1000);
    for (std::uint16_t v : {std::uint16_t{0}, std::uint16_t{48}, std::uint16_t{1046},
                            std::uint16_t{2047}}) {
        const auto f = encode(v, false);
        std::printf("value %4u -> frame 0x%04X = %s\n", v, f,
                    std::bitset<16>(f).to_string().c_str());
    }
    const auto f = encode(1046, true);
    std::printf("value 1046 with telemetry request -> 0x%04X\n", f);
    std::printf("first four pulse widths (ns):");
    for (int i = 0; i < 4; ++i) {
        std::printf(" %d", pulses(f)[i]);
    }
    std::printf("\n");

    std::uint16_t v = 0;
    bool tel = false;
    std::printf("decode clean frame: %s", decode(f, v, tel) ? "ok" : "REJECTED");
    std::printf(" (value %u, telemetry %d)\n", v, tel ? 1 : 0);
    int caught = 0;
    for (int bit = 0; bit < 16; ++bit) {
        const auto bad = static_cast<std::uint16_t>(f ^ (1u << bit));
        caught += decode(bad, v, tel) ? 0 : 1;
    }
    std::printf("single-bit errors caught by the checksum: %d of 16\n", caught);
    const auto two = static_cast<std::uint16_t>(f ^ (1u << 4) ^ (1u << 8));
    if (decode(two, v, tel)) {
        std::printf("two flipped bits (4 and 8): ACCEPTED as value %u, telemetry %d\n", v,
                    tel ? 1 : 0);
    } else {
        std::printf("two flipped bits (4 and 8): rejected\n");
    }
    return 0;
}

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>

// Forensic fix: write the agreed 6-byte format field by field, never the raw object.
struct SensorRecord
{
    std::uint8_t sensorId;
    std::uint32_t reading;
    std::uint8_t flags;
};

std::array<unsigned char, 6> encode(const SensorRecord& r)
{
    return {r.sensorId,
            static_cast<unsigned char>(r.reading & 0xFF),
            static_cast<unsigned char>((r.reading >> 8) & 0xFF),
            static_cast<unsigned char>((r.reading >> 16) & 0xFF),
            static_cast<unsigned char>((r.reading >> 24) & 0xFF),
            r.flags};
}

SensorRecord decode(const std::array<unsigned char, 6>& w)
{
    const std::uint32_t reading = std::uint32_t{w[1]} | (std::uint32_t{w[2]} << 8) |
                                  (std::uint32_t{w[3]} << 16) | (std::uint32_t{w[4]} << 24);
    return SensorRecord{w[0], reading, w[5]};
}

int main()
{
    const SensorRecord rec{3, 1000, 1};
    const auto wire = encode(rec);
    std::cout << "sender: bytes on the wire:";
    for (const unsigned char b : wire) {
        std::cout << ' ' << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(b);
    }
    std::cout << std::dec << " (" << wire.size() << " bytes)\n";
    const SensorRecord got = decode(wire);
    std::cout << "receiver: id " << unsigned{got.sensorId} << ", reading " << got.reading
              << ", flags " << unsigned{got.flags} << '\n';
    return 0;
}

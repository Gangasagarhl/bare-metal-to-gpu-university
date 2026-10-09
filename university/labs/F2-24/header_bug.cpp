#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>

// Forensic evidence: the sender's record and the format the receiver was promised.
struct SensorRecord
{
    std::uint8_t sensorId;
    std::uint32_t reading;
    std::uint8_t flags;
};

int main()
{
    const SensorRecord rec{3, 1000, 1};
    unsigned char wire[sizeof(SensorRecord)];
    std::memcpy(wire, &rec, sizeof rec);   // the sender transmits the raw object bytes

    std::cout << "sender: sizeof(SensorRecord) = " << sizeof(SensorRecord) << '\n';
    std::cout << "sender: bytes on the wire:";
    for (const unsigned char b : wire) {
        std::cout << ' ' << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(b);
    }
    std::cout << std::dec << '\n';

    // The receiver follows the written format: id (1 byte), reading (4 bytes, lowest byte
    // first), flags (1 byte) -- 6 bytes in total.
    const unsigned id = wire[0];
    const std::uint32_t reading = std::uint32_t{wire[1]} | (std::uint32_t{wire[2]} << 8) |
                                  (std::uint32_t{wire[3]} << 16) | (std::uint32_t{wire[4]} << 24);
    const unsigned flags = wire[5];
    std::cout << "receiver: id " << id << ", reading " << reading << ", flags " << flags << '\n';
    return 0;
}

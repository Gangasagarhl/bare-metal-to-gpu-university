// forensic_flat.cpp - evidence generator for the F10-20 forensic lab. A model of a
// flat-build firmware: one address space, data of different tasks side by side.
// It prints what the investigators had: an excerpt of the link map, the telemetry
// task's counters per second, the logger's messages, and a memory dump at the end.
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

std::array<std::uint8_t, 128> ram{};   // the simulated data memory, address 0x00..0x7f

constexpr std::size_t kLogLine = 0x40;     // char logger_line[32]
constexpr std::size_t kTelemRate = 0x60;   // uint8_t telem_rate_hz
constexpr std::size_t kTelemSeq = 0x61;    // uint8_t telem_seq (wraps)

void loggerWrite(const std::string& s)     // copies the text and its terminating zero
{
    for (std::size_t i = 0; i <= s.size(); ++i) {
        ram.at(kLogLine + i) = static_cast<std::uint8_t>(i < s.size() ? s[i] : '\0');
    }
}

} // namespace

int main()
{
    std::printf("link map excerpt (address, size, symbol):\n");
    std::printf("  0x%02zx  32  logger_line\n  0x%02zx   1  telem_rate_hz\n  0x%02zx   1  telem_seq\n\n",
                kLogLine, kTelemRate, kTelemSeq);
    ram.at(kTelemRate) = 10;   // telemetry: 10 packets per second
    const std::vector<std::string> log = {
        "boot ok", "baro ok", "IMU ok", "GPS: no fix", "GPS: 2D fix",
        "GPS: 3D fix, 9 satellites", "armed", "takeoff", "GPS: 3D fix, 11 sats, hdop 0.9",
        "GPS: 3D fix, 12 satellites, hdop", "hover", "hover",
    };
    std::printf("second  logger message                      telemetry packets sent\n");
    unsigned total = 0;
    for (std::size_t sec = 0; sec < log.size(); ++sec) {
        loggerWrite(log[sec]);
        const unsigned sent = ram.at(kTelemRate);   // the telemetry task sends rate_hz packets
        for (unsigned i = 0; i < sent; ++i) {
            ++ram.at(kTelemSeq);
        }
        total += sent;
        std::printf("%6zu  %-34s  %u\n", sec, log[sec].c_str(), sent);
    }
    std::printf("\ntotal packets %u\n\nmemory dump 0x40..0x63:\n", total);
    for (std::size_t a = 0x40; a < 0x64; a += 12) {
        std::printf("  0x%02zx:", a);
        std::string text;
        for (std::size_t i = a; i < a + 12 && i < 0x64; ++i) {
            std::printf(" %02x", ram.at(i));
            text += (ram.at(i) >= 0x20 && ram.at(i) < 0x7f) ? static_cast<char>(ram.at(i)) : '.';
        }
        std::printf("  |%s|\n", text.c_str());
    }
    return 0;
}

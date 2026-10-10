// grid_msg.hpp - MP8 starter lab: contract C-grid v1 between the perception kernel
// (producer, GPU code from MP3/MP4) and the robot's controller (consumer, from MP6).
// Every sentence of the contract is a rule below, and every rule has a contract test
// in contract_tests.cpp. The contract is our own teaching design, not a ROS 2 message.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace mp8 {

constexpr int kGridVersion = 1;        // C-grid v1
constexpr std::uint8_t kFree = 0;      // cell values: exactly these three
constexpr std::uint8_t kLethal = 100;  // robot centre here => robot body touches an obstacle
constexpr std::uint8_t kUnknown = 255; // no information; the consumer treats it as lethal

struct GridMsg
{
    int version = kGridVersion;
    std::uint32_t seq = 0;      // +1 per frame; never repeats; a gap (dropped frame) is allowed
    std::int64_t stampMs = 0;   // time the SENSOR data was taken (not when the kernel finished)
    int width = 0;              // cells in x
    int height = 0;             // cells in y
    int cellMm = 0;             // edge of one square cell, millimetres (integer: no unit guessing)
    std::vector<std::uint8_t> cells;   // row-major, cells[j * width + i]
};

struct Expect
{
    int width = 0;
    int height = 0;
    int cellMm = 0;
    std::int64_t maxAgeMs = 0;  // a frame older than this (now - stampMs) must not be used
};

// Consumer-side check. Returns "" when the frame may be used, otherwise the reason.
// lastSeq: sequence number of the last frame accepted (0 = none yet).
inline std::string check(const GridMsg& m, const Expect& e, std::uint32_t lastSeq, std::int64_t nowMs)
{
    if (m.version != kGridVersion) {
        return "version " + std::to_string(m.version) + " not understood (v1 only)";
    }
    if (m.width != e.width || m.height != e.height || m.cellMm != e.cellMm) {
        return "geometry differs from the agreed map";
    }
    if (m.cells.size() != static_cast<std::size_t>(m.width) * static_cast<std::size_t>(m.height)) {
        return "cell count does not match width x height";
    }
    for (std::uint8_t c : m.cells) {
        if (c != kFree && c != kLethal && c != kUnknown) {
            return "cell value " + std::to_string(c) + " is not 0, 100 or 255";
        }
    }
    if (lastSeq != 0 && m.seq <= lastSeq) {
        return "sequence " + std::to_string(m.seq) + " is not newer than " + std::to_string(lastSeq);
    }
    if (m.stampMs > nowMs) {
        return "stamp lies in the future";
    }
    if (nowMs - m.stampMs > e.maxAgeMs) {
        return "stale: age " + std::to_string(nowMs - m.stampMs) + " ms > " + std::to_string(e.maxAgeMs) + " ms";
    }
    return "";
}

}  // namespace mp8

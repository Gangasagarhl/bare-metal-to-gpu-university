// agent.cc - F9-51 Listing 2: the host side of the serial link. It plays the role that a
// micro-ROS agent plays for a real client (receive from the microcontroller, hand the data on
// to the rest of the robot), but it speaks only this course's own "uframe" format.
// Reads the firmware's UART output on stdin, checks every frame, decodes the IMU samples,
// and reports sequence gaps, checksum errors and the sample spacing.
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

uint16_t crc16(const std::vector<uint8_t>& b, std::size_t from, std::size_t n)
{
    uint16_t crc = 0xFFFF;                            // same algorithm as the firmware
    for (std::size_t i = from; i < from + n; ++i) {
        crc = static_cast<uint16_t>(crc ^ (b[i] << 8));
        for (int k = 0; k < 8; ++k) {
            crc = (crc & 0x8000) != 0 ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                                      : static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

uint16_t get16(const std::vector<uint8_t>& b, std::size_t i)
{
    return static_cast<uint16_t>(b[i] | (b[i + 1] << 8));
}

}  // namespace

int main()
{
    int frames = 0, bad = 0, shown = 0, gaps = 0;
    long missing = 0;
    long prevSeq = -1, prevTick = -1, minStep = 1 << 30, maxStep = 0;
    for (std::string line; std::getline(std::cin, line);) {
        if (line.rfind("F ", 0) != 0) {               // header and report lines: pass through
            std::printf("firmware says: %s\n", line.c_str());
            continue;
        }
        std::vector<uint8_t> b;
        for (std::size_t i = 2; i + 1 < line.size(); i += 2) {
            b.push_back(static_cast<uint8_t>(std::stoi(line.substr(i, 2), nullptr, 16)));
        }
        if (b.size() != 23 || b[0] != 0x7E || b[1] != 19 ||
            crc16(b, 1, 20) != get16(b, 21)) {
            ++bad;
            continue;
        }
        ++frames;
        const long seq = get16(b, 3);
        const long tick = static_cast<long>(get16(b, 5) | (static_cast<uint32_t>(get16(b, 7)) << 16));
        const auto s16 = [&](std::size_t i) { return static_cast<int16_t>(get16(b, i)); };
        if (shown < 3) {
            std::printf("imu seq %ld tick %ld accel_mg [%d %d %d] gyro_cdps [%d %d %d]\n", seq,
                        tick, s16(9), s16(11), s16(13), s16(15), s16(17), s16(19));
            ++shown;
        }
        if (prevSeq >= 0 && seq != prevSeq + 1) {
            ++gaps;
            missing += seq - prevSeq - 1;
            if (gaps <= 3) {
                std::printf("GAP: after seq %ld came seq %ld (%ld samples missing)\n", prevSeq,
                            seq, seq - prevSeq - 1);
            }
        }
        if (prevTick >= 0) {
            minStep = tick - prevTick < minStep ? tick - prevTick : minStep;
            maxStep = tick - prevTick > maxStep ? tick - prevTick : maxStep;
        }
        prevSeq = seq;
        prevTick = tick;
    }
    std::printf("frames ok %d, bad frames %d, sequence gaps %d, samples missing %ld\n", frames,
                bad, gaps, missing);
    std::printf("ticks between consecutive received samples: min %ld max %ld\n", minStep,
                maxStep);
    return 0;
}

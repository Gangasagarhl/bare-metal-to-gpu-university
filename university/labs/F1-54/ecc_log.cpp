// ecc_log.cpp - F1-54 forensic evidence generator: "the server that rebooted on Sunday".
// Prints a week of memory-error reports from a toy memory controller, in the simulator's
// own format (not the format of any real operating system), then the final event.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

static std::uint32_t next(std::uint32_t& s)
{
    s = s * 1664525u + 1013904223u;
    return s >> 8;
}

int main()
{
    std::uint32_t seed = 54;
    const int weakCellEvents[7] = {1, 1, 2, 3, 5, 8, 13};   // the weak cell fails more often
    std::printf("day hh:mm  type  channel dimm rank bank  row     col    bit  syndrome\n");
    for (int day = 1; day <= 7; ++day) {
        struct Line { unsigned minute; bool other; };
        std::vector<Line> lines;
        for (int e = 0; e < weakCellEvents[day - 1]; ++e) {
            lines.push_back({next(seed) % 1440, false});
        }
        if (day == 2 || day == 5) {            // unrelated single events elsewhere
            lines.push_back({next(seed) % 1440, true});
        }
        std::sort(lines.begin(), lines.end(), [](const Line& a, const Line& b) { return a.minute < b.minute; });
        for (const Line& l : lines) {
            if (!l.other) {
                std::printf("%3d %02u:%02u  CE    %7d %4d %4d %4d  0x1a2b  0x%03x  %4d  0x4b\n", day,
                            l.minute / 60, l.minute % 60, 1, 0, 0, 3, next(seed) % 0x400, 17);
            } else {
                std::printf("%3d %02u:%02u  CE    %7d %4d %4d %4u  0x%04x  0x%03x  %4u  0x%02x\n", day,
                            l.minute / 60, l.minute % 60, 0, 1, 1, next(seed) % 8, next(seed) % 0x10000,
                            next(seed) % 0x400, next(seed) % 64, next(seed) % 256);
            }
        }
    }
    std::printf("  7 23:58  UE    %7d %4d %4d %4d  0x1a2b  0x0c4  17,40  0x91  -> machine check, system reset\n",
                1, 0, 0, 3);
    std::printf("end of log\n");
    return 0;
}

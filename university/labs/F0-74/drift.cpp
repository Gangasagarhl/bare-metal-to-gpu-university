// F0-74 forensic evidence: a robot logger keeps its clock as "t += dt" in float (the deliberate
// mistake).
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

int main()
{
    const float dt = 0.01f; // 100 Hz control loop: one tick every 10 ms
    float t = 0.0f;         // the logger's time stamp, seconds
    std::printf("%-6s %-10s %-12s %-12s %-10s %s\n", "hours", "ticks", "true time s", "logged t s",
                "diff s", "spacing of floats near t (ms)");
    const int report[] = {1, 2, 3, 4, 6, 8, 12, 24};
    int next = 0;
    for (std::uint64_t tick = 1; tick <= 8640000; ++tick) { // 24 hours of ticks
        t += dt;
        if (next < 8 && tick == static_cast<std::uint64_t>(report[next]) * 360000u) {
            const double truth = static_cast<double>(tick) * 0.01;
            const float up = std::nextafter(t, std::numeric_limits<float>::infinity());
            std::printf("%-6d %-10llu %-12.2f %-12.4f %-10.4f %.4f\n", report[next],
                        static_cast<unsigned long long>(tick), truth, static_cast<double>(t),
                        static_cast<double>(t) - truth, static_cast<double>(up - t) * 1000.0);
            ++next;
        }
    }
    return 0;
}

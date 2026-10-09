// F1-66 Listing 2: two ways to turn encoder counts into speed.
// Pretend encoder: 400 counts per revolution after x4 decoding. Pretend timer: 1 MHz.
// "Count in a window" (frequency method) versus "time between edges" (period method).
#include <cmath>
#include <cstdio>
#include <initializer_list>

int main()
{
    const double cpr = 400.0;       // counts per revolution (pretend)
    const double window = 0.010;    // 10 ms counting window
    const double timerHz = 1.0e6;   // timer that stamps each edge (pretend)
    std::printf("resolution of the window method: 1 count / (cpr * window) = %.3f rev/s\n\n",
                1.0 / (cpr * window));
    std::printf("%10s | %-34s | %-30s\n", "true rev/s", "window method: counts -> rev/s",
                "period method: ticks -> rev/s");
    for (double speed : {0.10, 0.60, 2.00, 20.0, 150.0}) {
        // Edges happen every 1/(cpr*speed) seconds; the first edge at a phase offset.
        const double edgePeriod = 1.0 / (cpr * speed);
        // Window method: three consecutive windows.
        char wtext[64];
        int pos = 0;
        for (int w = 0; w < 3; ++w) {
            const double t0 = w * window;
            const double t1 = t0 + window;
            const double phase = 0.37 * edgePeriod;
            const long before = static_cast<long>(std::floor((t0 - phase) / edgePeriod));
            const long after = static_cast<long>(std::floor((t1 - phase) / edgePeriod));
            const long counts = after - before;
            pos += std::snprintf(wtext + pos, sizeof wtext - pos, "%ld->%.2f ", counts,
                                 counts / (cpr * window));
        }
        // Period method: ticks between two edges, rounded down by the timer.
        const long ticks = static_cast<long>(std::floor(edgePeriod * timerHz));
        const double est = ticks > 0 ? 1.0 / (cpr * ticks / timerHz) : 0.0;
        std::printf("%10.2f | %-34s | %ld -> %.4f\n", speed, wtext, ticks, est);
    }
    return 0;
}

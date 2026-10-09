// Clock arithmetic: period = 1 / frequency, and the timing budget of one clock cycle.
// All delays below are exercise values chosen for this chapter, not data of real parts.
#include <cstdio>
#include <string>
#include <vector>

struct Path
{
    std::string name;
    double clkToQ;   // ns: flip-flop output settles after the edge
    double logic;    // ns: slowest path through the logic between two flip-flops
    double setup;    // ns: input must be stable this long before the next edge
};

int main()
{
    // 1. Frequency to period: period in seconds = 1 / frequency in hertz.
    const std::vector<double> frequencies = {1.0, 1.0e3, 1.0e6, 100.0e6, 1.0e9};
    std::printf("%14s  %16s\n", "frequency (Hz)", "period (ns)");
    for (double f : frequencies) {
        std::printf("%14.0f  %16.3f\n", f, 1.0e9 / f);
    }

    // 2. Timing budget: the clock period must be at least clkToQ + logic + setup.
    const std::vector<Path> paths = {
        {"chapter simulation (ideal flip-flops, 7 ns adder)", 0.0, 7.0, 0.0},
        {"exercise A", 1.5, 6.0, 1.0},
        {"exercise B (logic made faster)", 1.5, 4.0, 1.0},
    };
    std::printf("\n%-50s %9s %14s\n", "path", "min period", "max frequency");
    for (const Path& p : paths) {
        const double minPeriodNs = p.clkToQ + p.logic + p.setup;
        const double maxFreqMHz = 1000.0 / minPeriodNs;   // 1 / ns = 1000 MHz
        std::printf("%-50s %6.1f ns %10.1f MHz\n", p.name.c_str(), minPeriodNs, maxFreqMHz);
    }

    // 3. Does a 6 ns clock work for the chapter simulation? (the forensic lab)
    const double periodNs = 6.0;
    const double needNs = paths[0].clkToQ + paths[0].logic + paths[0].setup;
    std::printf("\nperiod %.1f ns, path needs %.1f ns: %s (slack %.1f ns)\n", periodNs, needNs,
                periodNs >= needNs ? "OK" : "TOO FAST", periodNs - needNs);
    return 0;
}

// F10-11 forensic evidence generator "The battery that was emptier than the log said".
// Prints a once-a-minute excerpt of a flight's power log, the resting voltage after
// landing and the charger's report after recharging. SYNTHETIC: the pretend pack of
// battery_monitor.cpp (exercise values) and a fault that is described in the answer key.
#include <cstdio>

constexpr int kCells = 4;
constexpr double kCapacityAh = 3.0;
constexpr double kRpack = 0.040;

double ocvPerCell(double soc)
{
    const double s[] = {0.0, 0.2, 0.5, 0.8, 1.0};
    const double v[] = {3.30, 3.60, 3.75, 3.90, 4.10};
    for (int i = 0; i < 4; ++i) {
        if (soc <= s[i + 1]) {
            return v[i] + (v[i + 1] - v[i]) * (soc - s[i]) / (s[i + 1] - s[i]);
        }
    }
    return v[4];
}

int main()
{
    const double loggedScale = 14.0 / 20.0;   // the fault (see the answer key)
    double trueAh = 0, loggedAh = 0;
    std::printf("flight power log, one line per minute (pack fully charged at take-off)\n");
    std::printf("  t min  voltage V  current A  consumed mAh\n");
    for (int t = 0; t <= 480; ++t) {
        const double amps = 13.0 + ((t / 40) % 2 == 0 ? 0.0 : 2.0);   // gentle manoeuvres
        const double v = kCells * ocvPerCell(1.0 - trueAh / kCapacityAh) - amps * kRpack;
        if (t % 60 == 0) {
            std::printf("  %5d  %9.2f  %9.2f  %12.0f\n", t / 60, v, amps * loggedScale,
                        loggedAh * 1000);
        }
        trueAh += amps / 3600.0;
        loggedAh += amps * loggedScale / 3600.0;
    }
    const double rest = kCells * ocvPerCell(1.0 - trueAh / kCapacityAh);
    std::printf("after landing, 5 minutes at rest: %.2f V (%.3f V per cell)\n", rest,
                rest / kCells);
    std::printf("charger report after a full balance charge: %.0f mAh put back\n",
                trueAh * 1000 * 1.02);
    return 0;
}

// F10-11 Listing 2: why a battery warning based on raw voltage fires too early during a
// climb, and two better signals: voltage corrected for sag (V + I*R) and consumed charge.
// The pretend 4-cell pack: open-circuit voltage per cell from the table below, 3.0 Ah,
// pack internal resistance 0.040 ohm. These are EXERCISE VALUES, not limits for any
// real battery: real limits come from the battery maker's documentation.
#include <cstdio>

constexpr int kCells = 4;
constexpr double kCapacityAh = 3.0;
constexpr double kRpack = 0.040;            // ohm
constexpr double kWarnPerCell = 3.65;       // V per cell, the exercise warning level

double ocvPerCell(double soc)                // soc 0..1, linear between table points
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
    double usedAh = 0;
    bool rawWarned = false, sagWarned = false, mAhWarned = false;
    std::printf("  t s  current A  raw V  per cell  corrected/cell  used mAh  event\n");
    for (int t = 0; t <= 660; ++t) {                              // 11 minutes, 1 s steps
        const bool climb = (t % 120) >= 100 && (t % 120) < 106;    // 6 s climbs
        const double amps = climb ? 45.0 : 12.0;
        const double soc = 1.0 - usedAh / kCapacityAh;
        const double vRaw = kCells * ocvPerCell(soc) - amps * kRpack;
        const double perCell = vRaw / kCells;
        const double corrected = (vRaw + amps * kRpack) / kCells;  // R estimated in the lab
        const char* event = "";
        if (!rawWarned && perCell < kWarnPerCell) {
            rawWarned = true;
            event = "raw-voltage warning";
        }
        if (!sagWarned && corrected < kWarnPerCell) {
            sagWarned = true;
            event = "corrected-voltage warning";
        }
        if (!mAhWarned && usedAh > 0.8 * kCapacityAh) {
            mAhWarned = true;
            event = "used-charge warning (80 %)";
        }
        if (t % 60 == 0 || (t >= 100 && t < 108) || *event != '\0') {
            std::printf("%5d %10.1f %6.2f %9.3f %15.3f %9.0f  %s\n", t, amps, vRaw, perCell,
                        corrected, usedAh * 1000, event);
        }
        usedAh += amps / 3600.0;
    }
    return 0;
}

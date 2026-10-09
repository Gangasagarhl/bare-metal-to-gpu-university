// F1-72: analysis for the forensic key. Regenerates the same 400 rows as ground.cpp
// (same seed, same order of noise draws) and fits sensor V = v0 + slope x motor A
// by least squares, then repeats the model with the shared ground segment removed
// (sensor ground wired straight to the MCU ground: a "star" ground).
#include "noise.h"

#include <cmath>
#include <cstdio>
#include <initializer_list>

void analyse(const char* title, double rShared)
{
    Noise noise(7u);
    double sx = 0.0;
    double sy = 0.0;
    double sxx = 0.0;
    double sxy = 0.0;
    double syy = 0.0;
    const int n = 400;
    for (int k = 0; k < n; ++k) {
        const double t = k * 0.01;
        double cmd = 0.0;
        if (t >= 1.0 && t < 2.0) {
            cmd = 0.5;
        } else if (t >= 2.0 && t < 3.0) {
            cmd = 1.0;
        }
        const double amps = cmd * 3.0 + (cmd > 0.0 ? noise.gaussian(0.15) : 0.0);
        const double v = 1.000 + amps * rShared + noise.gaussian(0.002);
        sx += amps;
        sy += v;
        sxx += amps * amps;
        sxy += amps * v;
        syy += v * v;
    }
    const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    const double v0 = (sy - slope * sx) / n;
    const double r = (n * sxy - sx * sy) / std::sqrt((n * sxx - sx * sx) * (n * syy - sy * sy));
    std::printf("%-34s v0 = %.4f V, slope = %.4f V/A (= ohm), correlation r = %.3f\n", title,
                v0, slope, r);
}

int main()
{
    analyse("as wired (shared 0.05 ohm):", 0.05);
    analyse("star ground (shared part removed):", 0.0);
    return 0;
}

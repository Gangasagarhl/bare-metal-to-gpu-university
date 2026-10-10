// turn_stall.cpp - forensic evidence for F10-06 ("It fell out of the turn").
// The pretend aeroplane of fixed_wing.cpp flies level at 11 m/s, then the pilot banks
// steadily into a turn while the autopilot holds speed and height. Each row shows the lift
// coefficient the wing must produce for that. The log is the evidence.
#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double mass = 1.5, g = 9.81, rho = 1.225, S = 0.30, clMax = 1.2;
    const double speed = 11.0;
    const double clLevel = mass * g / (0.5 * rho * S * speed * speed);
    std::printf(" t(s)  bank(deg)  load factor  CL needed  CL max  status\n");
    for (int k = 0; k <= 16; ++k) {
        const double t = 0.5 * k;
        const double bank = std::fmin(10.0 * t, 70.0);  // 10 degrees per second, up to 70
        const double n = 1.0 / std::cos(bank * std::numbers::pi / 180.0);
        const double cl = n * clLevel;
        std::printf("%5.1f %10.1f %12.3f %10.3f %7.2f  %s\n", t, bank, n, cl, clMax,
                    cl > clMax ? "STALL: wing cannot make this lift" : "ok");
    }
    return 0;
}

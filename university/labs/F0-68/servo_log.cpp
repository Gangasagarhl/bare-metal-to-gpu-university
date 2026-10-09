// F0-68 forensic evidence generator ("The oscillating motor"): a motor positioning
// system receives a 1.0 rad step command at t = 0. Its angle is logged every 10 ms with
// a pretend encoder of 4096 counts per revolution. The system's parameters are given
// only in the answer key.
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>

int main()
{
    // closed-loop position dynamics: theta'' + 2 zeta wn theta' + wn^2 theta = wn^2 r
    const double zeta = 0.25;
    const double wn = 2.0 * std::numbers::pi * 4.0;   // rad/s
    const double dt = 1e-5;
    const double countsPerRad = 4096.0 / (2.0 * std::numbers::pi);
    double theta = 0.0;
    double omega = 0.0;
    std::cout << "# oscillating motor: step command 1.000 rad at t = 0; columns: time (s), angle (rad)\n";
    const int n = static_cast<int>(std::lround(0.6 / dt));
    for (int k = 0; k <= n; ++k) {
        if (k % 1000 == 0) {
            const double logged = std::round(theta * countsPerRad) / countsPerRad;
            std::cout << std::format("{:.3f} {:.4f}\n", k * dt, logged);
        }
        const double a = wn * wn * (1.0 - theta) - 2.0 * zeta * wn * omega;
        omega += a * dt;
        theta += omega * dt;
    }
    return 0;
}

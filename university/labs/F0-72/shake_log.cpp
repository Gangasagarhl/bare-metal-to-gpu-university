// F0-72 forensic evidence generator ("the shelf that shakes at one speed"): a fan on a
// wall shelf is run at a range of speeds; at each speed the vibration of the shelf is
// recorded once it is steady. The fan pushes with a force of the same size at every
// speed (a simplification stated in the scenario). The shelf's parameters are given
// only in the answer key.
#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>

int main()
{
    const double pi = std::numbers::pi;
    const double fn = 15.0;          // Hz
    const double zeta = 0.05;
    const double wn = 2.0 * pi * fn;
    std::cout << "# fan speed (rev/s = Hz), shelf vibration amplitude (mm), steady state\n";
    for (double f = 5.0; f <= 25.01; f += 1.0) {
        const double w = 2.0 * pi * f;
        const double period = 1.0 / f;
        const double dt = period / 2000.0;
        double x = 0.0;
        double v = 0.0;
        double amp = 0.0;
        const int periods = 200;
        for (int k = 0; k < periods * 2000; ++k) {
            const double t = k * dt;
            // x'' + 2 zeta wn x' + wn^2 x = wn^2 * (0.2 mm) * sin(w t)
            const double a = wn * wn * (0.2 * std::sin(w * t) - x) - 2.0 * zeta * wn * v;
            v += a * dt;
            x += v * dt;
            if (k >= (periods - 5) * 2000) {
                amp = std::max(amp, std::abs(x));
            }
        }
        std::cout << std::format("{:>4.0f} {:>7.3f}\n", f, amp);
    }
    return 0;
}

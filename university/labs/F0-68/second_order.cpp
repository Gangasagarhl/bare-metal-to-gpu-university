// F0-68 Listing 2: unit step responses of y'' + 2 zeta wn y' + wn^2 y = wn^2 u for several
// damping ratios, measured from a simulation and compared with the textbook formulas
//   overshoot Mp = exp(-pi zeta / sqrt(1 - zeta^2)),  peak time tp = pi / (wn sqrt(1 - zeta^2))
// (valid for 0 < zeta < 1) and the 2 % settling-time estimate 4 / (zeta wn).
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>

int main()
{
    const double wn = 10.0;     // rad/s
    const double dt = 1e-5;
    std::cout << "zeta | overshoot sim  formula | peak time sim  formula | 2% settle sim  4/(zeta wn)\n";
    for (double zeta : {0.1, 0.2, 0.5, 0.7, 1.0, 2.0}) {
        double y = 0.0;
        double v = 0.0;
        double peak = 0.0;
        double tPeak = 0.0;
        double tSettle = 0.0;
        const int n = static_cast<int>(std::lround(10.0 / dt));
        for (int k = 1; k <= n; ++k) {
            const double a = wn * wn * (1.0 - y) - 2.0 * zeta * wn * v;
            v += a * dt;          // semi-implicit Euler: new speed, then position
            y += v * dt;
            const double t = k * dt;
            if (y > peak) {
                peak = y;
                tPeak = t;
            }
            if (std::abs(y - 1.0) > 0.02) {
                tSettle = t;      // last time outside the 2 % band
            }
        }
        std::cout << std::format("{:>4.1f} | {:>12.1f}% ", zeta, 100.0 * std::max(0.0, peak - 1.0));
        if (zeta < 1.0) {
            const double wd = wn * std::sqrt(1.0 - zeta * zeta);
            const double mp = std::exp(-std::numbers::pi * zeta / std::sqrt(1.0 - zeta * zeta));
            std::cout << std::format("{:>7.1f}% | {:>11.3f} s {:>7.3f} s", 100.0 * mp, tPeak, std::numbers::pi / wd);
        } else {
            std::cout << "     -- |          --        --";
        }
        std::cout << std::format(" | {:>11.3f} s {:>9.3f} s\n", tSettle, 4.0 / (zeta * wn));
    }
    return 0;
}

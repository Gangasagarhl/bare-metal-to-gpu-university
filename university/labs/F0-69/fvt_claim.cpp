// F0-69 forensic evidence generator: a design note claimed that the system
//   y'' - y' - 2 y = 2 u,  y(0) = y'(0) = 0,  u = unit step,
// "settles at -1", using lim s->0 of s Y(s). This program prints that calculation and
// then simulates the system to see what it really does.
#include <cmath>
#include <format>
#include <iostream>

int main()
{
    // Y(s) = 2 / (s (s^2 - s - 2)); s Y(s) at s = 0 is 2 / (0 - 0 - 2) = -1
    std::cout << "design note: Y(s) = 2 / (s (s^2 - s - 2)); s Y(s) -> 2 / (-2) = -1 as s -> 0\n";
    const double dt = 1e-5;
    double y = 0.0;
    double v = 0.0;
    std::cout << "  t (s)          y(t)\n";
    const int n = static_cast<int>(std::lround(5.0 / dt));
    for (int k = 0; k <= n; ++k) {
        if (k % 50000 == 0) {
            std::cout << std::format("{:>7.1f} {:>13.4f}\n", k * dt, y);
        }
        const double a = 2.0 + v + 2.0 * y;
        y += v * dt;
        v += a * dt;
    }
    return 0;
}

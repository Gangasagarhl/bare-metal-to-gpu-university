// F0-64 Listing 3: position, velocity and acceleration of a braking cart.
// x(t) = 2 t - 0.25 t^2 (metres) for 0 <= t <= 4 s; the derivatives come from the rules.
#include <format>
#include <iostream>

int main()
{
    std::cout << "  t (s)   x (m)   v = x' (m/s)   a = v' (m/s^2)\n";
    for (int k = 0; k <= 8; ++k) {
        const double t = 0.5 * k;
        const double x = 2.0 * t - 0.25 * t * t;
        const double v = 2.0 - 0.5 * t;
        const double a = -0.5;
        std::cout << std::format("{:>7.1f} {:>7.3f} {:>14.3f} {:>16.3f}\n", t, x, v, a);
    }
    // The cart stops when v = 0: 2 - 0.5 t = 0, so t = 4 s.
    const double tStop = 2.0 / 0.5;
    std::cout << std::format("stops at t = {} s after travelling {} m\n", tStop, 2.0 * tStop - 0.25 * tStop * tStop);
    return 0;
}

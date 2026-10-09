// F0-67 Listing 2: a cart pushed to 1.5 m/s and released, with viscous friction c v and
// a constant (Coulomb) rolling friction Fc while it moves. For v > 0:
//   m v' = -c v - Fc   =>   v(t) = (v0 + Fc/c) e^(-t/tau) - Fc/c,   tau = m/c,
// and it stops for good at t_stop = tau ln(1 + c v0 / Fc).
#include <cmath>
#include <format>
#include <iostream>

int main()
{
    const double m = 2.0;
    const double c = 0.8;
    const double fc = 0.6;      // N, rolling friction while moving
    const double v0 = 1.5;      // m/s at release
    const double tau = m / c;
    const double tStop = tau * std::log(1.0 + c * v0 / fc);
    const double xStop = (v0 + fc / c) * tau * (1.0 - std::exp(-tStop / tau)) - fc / c * tStop;

    const double dt = 0.0001;
    double v = v0;
    double x = 0.0;
    double t = 0.0;
    while (v > 0.0) {
        const double a = (-c * v - fc) / m;
        x += v * dt;
        v += a * dt;
        t += dt;
    }
    std::cout << std::format("analytic:   stops at t = {:.4f} s after {:.4f} m\n", tStop, xStop);
    std::cout << std::format("simulated:  stops at t = {:.4f} s after {:.4f} m (dt = {} s)\n", t, x, dt);
    std::cout << std::format("with viscous friction only, the cart would never quite stop; it would roll v0 tau = {:.4f} m\n",
                             v0 * tau);
    return 0;
}

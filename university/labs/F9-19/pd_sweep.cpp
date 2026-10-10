// F9-19 Listing 1: position control of the cart (m = 2 kg, friction c = 0.8 N s/m,
// MA301 F0-67) with a fixed proportional gain and five derivative gains.
// Setpoint step 0 -> 1 m; controller every 10 ms; force limited to +-10 N.
// The derivative term acts on the measured position (the cart's velocity estimate).
#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>

struct Cart
{
    double mass = 2.0;     // kg
    double friction = 0.8; // N per (m/s)
    double x = 0.0;        // m
    double v = 0.0;        // m/s

    void step(double force, double dt)
    {
        v += dt * (force - friction * v) / mass;
        x += dt * v;
    }
};

struct Result
{
    double overshootPercent;
    double settlingTime; // last time outside +-2 % of the setpoint
    double peakForce;
};

Result run(double kp, double kd)
{
    const double setpoint = 1.0;
    const double period = 0.01;
    Cart cart;
    double previousX = 0.0;
    Result r{0.0, 0.0, 0.0};
    double peak = 0.0;
    for (int k = 0; k < 2000; ++k) { // 20 s
        const double t = k * period;
        const double x = cart.x;
        const double dTerm = -kd * (x - previousX) / period;
        previousX = x;
        const double force = std::clamp(kp * (setpoint - x) + dTerm, -10.0, 10.0);
        r.peakForce = std::max(r.peakForce, std::abs(force));
        peak = std::max(peak, x);
        if (std::abs(x - setpoint) > 0.02 * setpoint) {
            r.settlingTime = t + period;
        }
        for (int s = 0; s < 100; ++s) {
            cart.step(force, period / 100);
        }
    }
    r.overshootPercent = 100.0 * std::max(0.0, peak - setpoint) / setpoint;
    return r;
}

int main()
{
    const double kp = 8.0; // N per m
    std::cout << "Kp = 8 N/m; zeta = (c + Kd) / (2 sqrt(Kp m)) for the continuous-time model\n";
    std::cout << "  Kd (N s/m)   zeta   predicted overshoot   simulated overshoot   settling (2 %) "
                 "  peak force\n";
    for (double kd : {0.0, 1.6, 4.8, 7.2, 20.0}) {
        const double zeta = (0.8 + kd) / (2.0 * std::sqrt(kp * 2.0));
        const double predicted =
            zeta < 1.0 ? 100.0 * std::exp(-3.141592653589793 * zeta / std::sqrt(1.0 - zeta * zeta))
                       : 0.0;
        const Result r = run(kp, kd);
        std::cout << std::format(
            "{:>12.1f} {:>6.2f} {:>19.1f} % {:>19.1f} % {:>14.2f} s {:>10.2f} N\n", kd, zeta,
            predicted, r.overshootPercent, r.settlingTime, r.peakForce);
    }
    return 0;
}

// F10-09 Listing 1: a soft mount as a mass on a spring and damper, shaken by the frame.
// Simulates x'' = -2*zeta*wn*(x' - y') - wn^2*(x - y) with y = sin(w t) (RK4), measures the
// steady amplitude of x, and compares it with the transmissibility formula
// T(r) = sqrt(1 + (2 zeta r)^2) / sqrt((1 - r^2)^2 + (2 zeta r)^2), r = f / fn.
// fn and zeta are exercise values, not properties of any real damper.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

constexpr double kFn = 25.0;     // Hz, natural frequency of the mount (exercise value)
constexpr double kZeta = 0.15;   // damping ratio (exercise value)

double simulatedRatio(double f)
{
    const double wn = 2 * std::numbers::pi * kFn, w = 2 * std::numbers::pi * f;
    auto acc = [&](double t, double x, double v) {
        const double y = std::sin(w * t), yd = w * std::cos(w * t);
        return -2 * kZeta * wn * (v - yd) - wn * wn * (x - y);
    };
    const double dt = 1e-5, tEnd = 2.0;
    double x = 0, v = 0, peak = 0;
    for (double t = 0; t < tEnd; t += dt) {
        const double k1x = v, k1v = acc(t, x, v);
        const double k2x = v + 0.5 * dt * k1v, k2v = acc(t + 0.5 * dt, x + 0.5 * dt * k1x, k2x);
        const double k3x = v + 0.5 * dt * k2v, k3v = acc(t + 0.5 * dt, x + 0.5 * dt * k2x, k3x);
        const double k4x = v + dt * k3v, k4v = acc(t + dt, x + dt * k3x, k4x);
        x += dt / 6 * (k1x + 2 * k2x + 2 * k3x + k4x);
        v += dt / 6 * (k1v + 2 * k2v + 2 * k3v + k4v);
        if (t > tEnd - 0.5) {
            peak = std::max(peak, std::abs(x));   // steady state: last 0.5 s only
        }
    }
    return peak;   // the frame moves with amplitude 1
}

double formulaRatio(double f)
{
    const double r = f / kFn, d = 2 * kZeta * r;
    return std::sqrt(1 + d * d) / std::sqrt((1 - r * r) * (1 - r * r) + d * d);
}

int main()
{
    std::printf("mount: fn = %.0f Hz, zeta = %.2f\n", kFn, kZeta);
    std::printf("  f Hz   f/fn  simulated  formula   meaning\n");
    for (double f : {5.0, 15.0, 25.0, 35.36, 50.0, 100.0, 190.0}) {
        const double s = simulatedRatio(f), F = formulaRatio(f);
        std::printf("%6.2f %6.2f %10.3f %8.3f   %s\n", f, f / kFn, s, F,
                    F > 1.01 ? "amplified" : (F < 0.99 ? "isolated" : "crossover, r = sqrt(2)"));
    }
    return 0;
}

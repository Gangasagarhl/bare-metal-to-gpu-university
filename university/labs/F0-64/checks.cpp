// F0-64 number check: recomputes the numbers used in the chapter text.
#include <cmath>
#include <format>
#include <iostream>

double x(double t)
{
    return 0.3 * t * t + 0.5 * t;
}

int main()
{
    std::cout << std::format("hook: x(1) = {:.2f}, x(2) = {:.2f}, x(3) = {:.2f}; gaps {:.2f}, {:.2f}, {:.2f}\n", x(1), x(2), x(3), x(1) - x(0),
                             x(2) - x(1), x(3) - x(2));
    for (double h : {1.0, 0.1, 0.01}) {
        std::cout << std::format("quotient at t = 1, h = {}: {:.6f} (1.1 + 0.3h = {:.6f})\n", h, (x(1 + h) - x(1)) / h,
                                 1.1 + 0.3 * h);
    }
    std::cout << std::format("braking cart: v(0) = {}, stop t = {}, x(4) = {}\n", 2.0, 2.0 / 0.5, 2.0 * 4 - 0.25 * 16);
    const double kdc = 0.01 / (0.01 * 0.01 + 2.0 * 1e-6);
    const double tau = 1e-5 * 2.0 / (0.01 * 0.01 + 2.0 * 1e-6);
    const double wInf = kdc * 6.0;
    std::cout << std::format("motor: w_inf = {:.2f} rad/s, w'(0) = {:.1f} rad/s^2, w'(tau) = {:.1f} rad/s^2\n", wInf,
                             wInf / tau, wInf / tau * std::exp(-1.0));
    std::cout << std::format("exact motor start: K V / (J R) = {:.1f} rad/s^2\n", 0.01 * 6.0 / (1e-5 * 2.0));
    std::cout << std::format("Q1: {:.2f} C/s; Q4: v = 0 at t = {:.2f} s; Q6: v = {:.2f} m/s\n", (50.0 - 20.0) / 60.0, 1.2 / 0.2,
                             0.03 * 8.0 * 0.5);
    for (double h : {0.1, 0.01, 0.001}) {
        std::cout << std::format("Q8: t^3 at 1, h = {}: quotient {:.6f}, error {:.6f}\n", h,
                                 (std::pow(1 + h, 3) - 1) / h, (std::pow(1 + h, 3) - 1) / h - 3.0);
    }
    return 0;
}

// F0-66 number check: recomputes the numbers used in the chapter text.
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>

int main()
{
    const double pi = std::numbers::pi;
    std::cout << std::format("constants: x''(1)/2 = {:.4f}, x'''(1)/6 = {:.5f}\n", -0.5 * std::sin(1.0),
                             -std::cos(1.0) / 6.0);
    const double q = 2.0 * pi / 4096.0;
    std::cout << std::format("q = {:.7f} rad; q/dt = {:.4f} rad/s; W = 20: {:.4f} rad/s\n", q, q / 0.001,
                             q / 0.02);
    std::cout << std::format("true speed at t = 2.25: {:.4f} rad/s = {:.3f} counts per sample\n",
                             pi * std::cos(pi * 2.25), pi * std::cos(pi * 2.25) * 0.001 / q);
    std::cout << std::format("alpha: backward {:.6f}, exact hold {:.6f}, forward {:.6f}\n", 0.001 / 0.011,
                             1.0 - std::exp(-0.1), 0.1);
    const double a = 0.001 / 0.011;
    std::cout << std::format("worked: W > {:.2f}; (1-a)^100 = {:.2e}; after 10: {:.4f}; after 11: {:.4f}; 1-e^-1 = {:.4f}\n",
                             (q / 0.001) / 0.1, std::pow(1 - a, 100), 1 - std::pow(1 - a, 10), 1 - std::pow(1 - a, 11),
                             1 - std::exp(-1.0));
    std::cout << std::format("corner 1/(2 pi tau), tau = 10 ms: {:.1f} Hz\n", 1.0 / (2 * pi * 0.01));
    std::cout << std::format("Q1: {:.1f} {:.1f} {:.1f}; Q3: {:.4f} rad/s; Q4: alpha {:.4f}; Q6: {:.1f} C/s\n",
                             (3.2 - 2.5) / 0.1, (2.5 - 2.0) / 0.1, (3.2 - 2.0) / 0.2, 2 * pi / 1000 / 0.002, 5.0 / 55.0,
                             0.5 / 0.1);
    std::cout << std::format("forensic: 0.05 * q / 0.001 = {:.4f} V; counts/s at 0.3 rad/s = {:.1f}; mean D = {:.4f} V\n",
                             0.05 * q / 0.001, 0.3 / q, 4 * 0.0767 / 20.0);
    std::cout << std::format("forensic fix: 3 or 4 counts per 20 ms -> D = {:.4f} or {:.4f} V\n", 0.05 * 3 * q / 0.02,
                             0.05 * 4 * q / 0.02);
    return 0;
}

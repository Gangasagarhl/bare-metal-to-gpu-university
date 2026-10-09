// F0-65 number check: recomputes the numbers used in the chapter text.
#include <cmath>
#include <format>
#include <iostream>

double v(double t)
{
    return 2.0 * (1.0 - std::exp(-t / 2.0));
}

int main()
{
    const double exact = 4.0 + 4.0 * std::exp(-2.0);
    std::cout << std::format("e^-2 = {:.6f}; distance = {:.4f} m; average speed = {:.4f} m/s\n", std::exp(-2.0), exact,
                             exact / 4.0);
    std::cout << std::format("v(0..4) = {:.5f} {:.5f} {:.5f} {:.5f} {:.5f}\n", v(0), v(1), v(2), v(3), v(4));
    const double t4 = 0.5 * v(0) + v(1) + v(2) + v(3) + 0.5 * v(4);
    std::cout << std::format("trapezoid N = 4: {:.4f} m, error {:.4f} m\n", t4, t4 - exact);
    std::cout << std::format("energy 1.0..2.0 s: {:.2f} J\n", 0.5 * (3.2 + 2.0) / 2.0 + 0.5 * (2.0 + 1.0) / 2.0);
    std::cout << std::format("Q1: {} L; Q2: {}; Q4: {} C = {:.1f} mAh; Q6: {:.2f} rad = {:.1f} deg; Q7: {}\n",
                             6 * 2 + 3 * 4, 8 + 2, 2 * 30 + 0.5 * 60, (2 * 30 + 0.5 * 60) / 3.6, 0.002 * 300,
                             0.002 * 300 * 180.0 / 3.141592653589793, 3.0 / 2.0);
    std::cout << std::format("forensic: parked growth {:.4f} m/s; driving 0.4 * 5 = {:.3f} m, error {:.3f} m\n",
                             (4.420 - 2.260) / (605.0 - 65.0), 0.4 * 5, 0.004 * 5);
    std::cout << std::format("lab: -0.001 m/s for 600 s = {:.1f} m\n", -0.001 * 600);
    return 0;
}

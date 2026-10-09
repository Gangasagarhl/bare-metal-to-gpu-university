// F0-69 number check: recomputes the numbers used in the chapter text.
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>

int main()
{
    // (a) Y = (s + 4) / (s (s + 2)): cover-up coefficients.
    const double aA = (0.0 + 4.0) / (0.0 + 2.0);
    const double aB = (-2.0 + 4.0) / (-2.0);
    std::cout << std::format("(a) A = {:.4f}, B = {:.4f}  ->  y = A + B e^(-2t)\n", aA, aB);
    // (b) Y = 2 / (s (s + 1) (s + 2)).
    const double bA = 2.0 / (1.0 * 2.0);
    const double bB = 2.0 / (-1.0 * 1.0);
    const double bC = 2.0 / (-2.0 * -1.0);
    std::cout << std::format("(b) A = {:.4f}, B = {:.4f}, C = {:.4f}\n", bA, bB, bC);
    // Worked example: pretend motor, first-order form, 6 V step.
    const double tau = 1e-5 * 2.0 / (0.01 * 0.01 + 2.0 * 1e-6);
    const double kdc = 0.01 / (0.01 * 0.01 + 2.0 * 1e-6);
    std::cout << std::format("motor: tau = {:.5f} s, 1/tau = {:.3f} 1/s, Kdc = {:.3f} rad/s per V\n", tau, 1.0 / tau,
                             kdc);
    std::cout << std::format("motor: final value 6 Kdc = {:.2f} rad/s, initial slope 6 Kdc / tau = {:.1f} rad/s^2\n",
                             6.0 * kdc, 6.0 * kdc / tau);
    // Question 6: cart 2 v' + 0.8 v = 1.6, v(0) = 0.5.
    const double cA = 0.5 * 1.6 / 0.4;
    const double cB = 0.5 * (-0.4 + 1.6) / (-0.4);
    std::cout << std::format("Q6: v = {:.4f} + ({:.4f}) e^(-0.4 t)\n", cA, cB);
    // Lab (d): y'' + 2 y' + 5 y = 5.
    const double wn = std::sqrt(5.0);
    const double zeta = 2.0 / (2.0 * wn);
    const double os = std::exp(-zeta * std::numbers::pi / std::sqrt(1.0 - zeta * zeta));
    std::cout << std::format("lab (d): wn = {:.4f}, zeta = {:.4f}, overshoot = {:.2f} % at t = {:.4f} s\n", wn, zeta,
                             100.0 * os, std::numbers::pi / (wn * std::sqrt(1.0 - zeta * zeta)));
    // Forensic: Y = 2 / (s (s - 2) (s + 1)).
    const double fA = 2.0 / ((-2.0) * 1.0);
    const double fB = 2.0 / (2.0 * 3.0);
    const double fC = 2.0 / ((-1.0) * (-3.0));
    std::cout << std::format("forensic: A = {:.4f}, B = {:.4f}, C = {:.4f}\n", fA, fB, fC);
    for (double t : {1.0, 2.0, 5.0}) {
        const double y = fA + fB * std::exp(2.0 * t) + fC * std::exp(-t);
        std::cout << std::format("forensic exact y({}) = {:.4f}  (terms {:.4f}, {:.4f}, {:.4f})\n", t, y, fA,
                                 fB * std::exp(2.0 * t), fC * std::exp(-t));
    }
    return 0;
}

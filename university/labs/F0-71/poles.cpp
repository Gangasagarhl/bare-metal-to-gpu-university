// F0-71 Listing 1: poles of a motor speed loop with proportional gain Kp.
// Loop: controller Kp, driver lag 1/(0.01 s + 1), motor 98.039/(0.19608 s + 1),
// speed filter 1/(0.02 s + 1). Characteristic polynomial (negative feedback):
//   (tm s + 1)(ta s + 1)(tf s + 1) + Kp Kdc = a3 s^3 + a2 s^2 + a1 s + a0.
// Roots are found with the Durand-Kerner iteration; the Routh-Hurwitz test for a cubic
// (all a > 0 and a2 a1 > a3 a0) gives the critical gain.
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <format>
#include <iostream>

using Complex = std::complex<double>;

std::array<Complex, 3> cubicRoots(double a3, double a2, double a1, double a0)
{
    const double b2 = a2 / a3;
    const double b1 = a1 / a3;
    const double b0 = a0 / a3;
    auto p = [&](Complex z) { return ((z + b2) * z + b1) * z + b0; };
    std::array<Complex, 3> z = {Complex(0.4, 0.9), Complex(0.4, 0.9) * Complex(0.4, 0.9),
                                Complex(0.4, 0.9) * Complex(0.4, 0.9) * Complex(0.4, 0.9)};
    const double scale = 1.0 + std::abs(b2) + std::abs(b1) + std::abs(b0);
    for (Complex& zi : z) {
        zi *= scale;
    }
    for (int iter = 0; iter < 500; ++iter) {
        for (std::size_t i = 0; i < 3; ++i) {
            Complex denom = 1.0;
            for (std::size_t j = 0; j < 3; ++j) {
                if (j != i) {
                    denom *= (z[i] - z[j]);
                }
            }
            z[i] -= p(z[i]) / denom;
        }
    }
    return z;
}

int main()
{
    const double tm = 0.19608;
    const double ta = 0.01;
    const double tf = 0.02;
    const double kdc = 98.039;
    const double a3 = tm * ta * tf;
    const double a2 = tm * ta + tm * tf + ta * tf;
    const double a1 = tm + ta + tf;
    const double kpCrit = (a2 * a1 / a3 - 1.0) / kdc;
    std::cout << std::format("a3 = {:.4e}, a2 = {:.4e}, a1 = {:.5f}\n", a3, a2, a1);
    std::cout << std::format("Routh-Hurwitz: stable while 1 + Kp Kdc < a2 a1 / a3 = {:.3f}, so Kp < {:.4f}\n",
                             a2 * a1 / a3, kpCrit);
    std::cout << std::format("at the critical gain the poles cross at s = +/- j {:.2f} rad/s ({:.2f} Hz)\n",
                             std::sqrt(a1 / a3), std::sqrt(a1 / a3) / (2.0 * 3.141592653589793));
    std::cout << "    Kp   poles (1/s)                                                  verdict\n";
    for (double kp : {0.0, 0.01, 0.05, 0.1, 0.2, kpCrit, 0.5}) {
        const auto r = cubicRoots(a3, a2, a1, 1.0 + kp * kdc);
        double maxReal = -1e300;
        std::cout << std::format("{:>6.4f} ", kp);
        for (const Complex& root : r) {
            std::cout << std::format("  {:>8.2f} {:+8.2f}j", root.real(), std::abs(root.imag()) < 1e-6 ? 0.0 : root.imag());
            maxReal = std::max(maxReal, root.real());
        }
        const char* verdict = maxReal < -1e-3 ? "stable" : (maxReal > 1e-3 ? "UNSTABLE" : "on the edge");
        std::cout << "   " << verdict << "\n";
    }
    return 0;
}

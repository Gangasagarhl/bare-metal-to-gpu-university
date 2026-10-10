// F0-72 Listing 2: Bode magnitude (dB) and phase (degrees) computed from transfer
// functions at frequencies spaced evenly on a log scale:
//   first order   G1(s) = 1 / (0.1 s + 1)              (corner at 10 rad/s)
//   second order  G2(s) = wn^2 / (s^2 + 2 zeta wn s + wn^2), wn = 10 rad/s, zeta = 0.1, 0.3, 0.7
#include <cmath>
#include <complex>
#include <format>
#include <iostream>
#include <numbers>

using Complex = std::complex<double>;

int main()
{
    const double rad2deg = 180.0 / std::numbers::pi;
    const double wn = 10.0;
    std::cout << "  w (rad/s) |  G1 dB  G1 deg | z=0.1 dB   deg | z=0.3 dB   deg | z=0.7 dB   deg\n";
    for (int k = -4; k <= 8; ++k) {
        const double w = 10.0 * std::pow(10.0, k / 4.0);
        const Complex s(0.0, w);
        const Complex g1 = 1.0 / (0.1 * s + 1.0);
        std::cout << std::format("{:>11.2f} | {:>6.1f} {:>7.1f}", w, 20.0 * std::log10(std::abs(g1)),
                                 std::arg(g1) * rad2deg);
        for (double zeta : {0.1, 0.3, 0.7}) {
            const Complex g2 = wn * wn / (s * s + 2.0 * zeta * wn * s + wn * wn);
            std::cout << std::format(" | {:>8.1f} {:>6.1f}", 20.0 * std::log10(std::abs(g2)), std::arg(g2) * rad2deg);
        }
        std::cout << "\n";
    }
    for (double zeta : {0.1, 0.3, 0.7}) {
        if (zeta < 1.0 / std::sqrt(2.0)) {
            const double wr = wn * std::sqrt(1.0 - 2.0 * zeta * zeta);
            const double mr = 1.0 / (2.0 * zeta * std::sqrt(1.0 - zeta * zeta));
            std::cout << std::format("zeta = {}: resonant peak {:.3f} ({:.1f} dB) at {:.2f} rad/s\n", zeta, mr,
                                     20.0 * std::log10(mr), wr);
        } else {
            std::cout << std::format("zeta = {}: no resonant peak (zeta >= 1/sqrt 2)\n", zeta);
        }
    }
    return 0;
}

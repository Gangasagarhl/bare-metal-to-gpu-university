// heading_margin.cpp - how large a heading error can the course position loop survive?
// Linear model of the horizontal loop of dronesim.hpp (limits ignored), written for one
// complex coordinate p = x + i*y. A heading error delta rotates every command by e^{i delta}:
//   p' = v,  v' = a - drag*v,  tau*a' = e^{i delta} * kv * (kp*(0 - p) - v) - a
// Characteristic polynomial: tau s^3 + (1 + tau drag) s^2 + (drag + E kv) s + E kv kp = 0,
// E = e^{i delta}. The loop is stable when every root has a negative real part.
#include <array>
#include <cmath>
#include <complex>
#include <cstdio>

using C = std::complex<double>;

std::array<C, 3> roots(C a3, C a2, C a1, C a0)  // Durand-Kerner on the monic cubic
{
    const C b2 = a2 / a3, b1 = a1 / a3, b0 = a0 / a3;
    auto f = [&](C s) { return ((s + b2) * s + b1) * s + b0; };
    std::array<C, 3> r = {C(0.4, 0.9), C(0.4, 0.9) * C(0.4, 0.9),
                          C(0.4, 0.9) * C(0.4, 0.9) * C(0.4, 0.9)};
    for (int it = 0; it < 500; ++it)
        for (int i = 0; i < 3; ++i) {
            C den = 1;
            for (int j = 0; j < 3; ++j)
                if (j != i) den *= (r[i] - r[j]);
            r[i] -= f(r[i]) / den;
        }
    return r;
}

double max_real(double delta_deg)
{
    const double kp = 0.9, kv = 1.8, tau = 0.15, drag = 0.35, pi = std::acos(-1.0);
    const C E = std::polar(1.0, delta_deg * pi / 180.0);
    const auto r = roots(tau, 1.0 + tau * drag, drag + E * kv, E * kv * kp);
    double m = -1e9;
    for (const C& z : r) m = std::max(m, z.real());
    return m;
}

int main()
{
    std::printf("%12s %22s %s\n", "delta [deg]", "max Re(root) [1/s]", "verdict");
    for (int d = 0; d <= 120; d += 10) {
        const double m = max_real(d);
        std::printf("%12d %22.4f %s\n", d, m, m < 0 ? "stable" : "UNSTABLE (spiral grows)");
    }
    double lo = 0, hi = 120;  // bisection for the boundary
    for (int i = 0; i < 60; ++i) {
        const double mid = 0.5 * (lo + hi);
        (max_real(mid) < 0 ? lo : hi) = mid;
    }
    std::printf("stability boundary: delta = %.1f deg (same for -delta by symmetry)\n", lo);
    return 0;
}

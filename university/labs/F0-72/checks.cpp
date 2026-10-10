// F0-72 number check: recomputes the numbers used in the chapter text.
#include <cmath>
#include <complex>
#include <format>
#include <iostream>
#include <numbers>

using Complex = std::complex<double>;

int main()
{
    const double pi = std::numbers::pi;
    const double tm = 0.19608;
    const double kdc = 98.039;
    // First-order motor: corner, gain and phase there.
    std::cout << std::format("motor corner 1/tau = {:.3f} rad/s ({:.3f} Hz), DC gain {:.2f} dB, at corner {:.3f} = "
                             "{:.2f} dB\n",
                             1.0 / tm, 1.0 / tm / (2.0 * pi), 20.0 * std::log10(kdc), kdc / std::sqrt(2.0),
                             20.0 * std::log10(kdc / std::sqrt(2.0)));
    // Speed loop of F0-71: L(jw) = Kp Kdc / ((tm s + 1)(ta s + 1)(tf s + 1)).
    const double ta = 0.01;
    const double tf = 0.02;
    auto loop = [&](double kp, double w) {
        const Complex s(0.0, w);
        return kp * kdc / ((tm * s + 1.0) * (ta * s + 1.0) * (tf * s + 1.0));
    };
    const double wpc = std::sqrt((tm + ta + tf) / (tm * ta * tf));
    const Complex lpc = loop(0.05, wpc);
    double phasePc = std::arg(lpc) * 180.0 / pi;
    if (phasePc > 0.0) {
        phasePc -= 360.0;   // atan2 returns +180 for a phase of exactly -180
    }
    std::cout << std::format("phase crossover {:.2f} rad/s: phase {:.2f} deg, |L| at Kp = 0.05 = {:.4f}, gain margin "
                             "x{:.2f} = {:.1f} dB\n",
                             wpc, phasePc, std::abs(lpc), 1.0 / std::abs(lpc),
                             -20.0 * std::log10(std::abs(lpc)));
    // Gain crossover at Kp = 0.05 by bisection on |L| = 1, then phase margin.
    double lo = 1.0;
    double hi = wpc;
    for (int i = 0; i < 100; ++i) {
        const double mid = 0.5 * (lo + hi);
        (std::abs(loop(0.05, mid)) > 1.0 ? lo : hi) = mid;
    }
    std::cout << std::format("gain crossover at Kp = 0.05: {:.2f} rad/s, phase {:.1f} deg, phase margin {:.1f} deg\n",
                             lo, std::arg(loop(0.05, lo)) * 180.0 / pi, 180.0 + std::arg(loop(0.05, lo)) * 180.0 / pi);
    // Second-order resonance and slopes.
    for (double zeta : {0.05, 0.1, 0.3}) {
        std::cout << std::format("zeta = {}: |G| at wn = 1/(2 zeta) = {:.3f} ({:.1f} dB)\n", zeta, 1.0 / (2.0 * zeta),
                                 20.0 * std::log10(1.0 / (2.0 * zeta)));
    }
    // Shelf: static deflection 0.2 mm, fn = 15 Hz, zeta = 0.05.
    for (double f : {5.0, 14.0, 15.0, 16.0, 25.0}) {
        const double r = f / 15.0;
        const double amp = 0.2 / std::sqrt((1.0 - r * r) * (1.0 - r * r) + (2.0 * 0.05 * r) * (2.0 * 0.05 * r));
        std::cout << std::format("shelf exact amplitude at {} Hz = {:.3f} mm\n", f, amp);
    }
    std::cout << std::format("shelf: half-power band 2 zeta fn = {:.2f} Hz; peak/static = {:.2f} -> zeta = {:.3f}\n",
                             2.0 * 0.05 * 15.0, 2.000 / 0.2, 1.0 / (2.0 * 2.000 / 0.2));
    std::cout << std::format("shelf: 5 Hz reading 0.225 mm / (1/(1 - (5/15)^2)) = {:.3f} mm static\n",
                             0.225 * (1.0 - 1.0 / 9.0));
    // Exponential moving average of F0-66 as a first-order low-pass: corner 1/tf for tf = 20 ms.
    std::cout << std::format("filter tf = 20 ms: corner {:.1f} rad/s ({:.2f} Hz)\n", 1.0 / tf, 1.0 / tf / (2.0 * pi));
    return 0;
}

// F0-68 number check: recomputes the numbers used in the chapter text.
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>

struct Roots
{
    double slow;
    double fast;
};

// Real roots of L J s^2 + (R J + L b) s + (R b + K^2) = 0 (over-damped case).
Roots motorRoots(double r, double l, double k, double j, double b)
{
    const double a2 = l * j;
    const double a1 = r * j + l * b;
    const double a0 = r * b + k * k;
    const double disc = std::sqrt(a1 * a1 - 4.0 * a2 * a0);
    return {(-a1 + disc) / (2.0 * a2), (-a1 - disc) / (2.0 * a2)};
}

// Time for the full motor model to reach 63.2 % of its final speed, and the current then.
double time63(double l, double j, double& currentAt)
{
    const double r = 2.0;
    const double k = 0.01;
    const double b = 1e-6;
    const double wFinal = 6.0 * k / (k * k + r * b);
    double i = 0.0;
    double w = 0.0;
    const double dt = 1e-6;
    for (int s = 1; s < 5000000; ++s) {
        const double di = (6.0 - r * i - k * w) / l;
        const double dw = (k * i - b * w) / j;
        i += di * dt;
        w += dw * dt;
        if (w >= (1.0 - std::exp(-1.0)) * wFinal) {
            currentAt = i;
            return s * dt;
        }
    }
    return -1.0;
}

int main()
{
    const double pi = std::numbers::pi;
    const double r = 2.0;
    const double l = 1e-3;
    const double k = 0.01;
    const double j = 1e-5;
    const double b = 1e-6;
    const double den = k * k + r * b;
    std::cout << std::format("K^2 + R b = {:.3e}; tau_m = {:.4f} s; Kdc = {:.2f}; 6 V -> {:.1f} rad/s = {:.0f} rpm; 3 tau = {:.3f} s\n",
                             den, j * r / den, k / den, 6 * k / den, 6 * k / den * 60 / (2 * pi), 3 * j * r / den);
    const Roots m = motorRoots(r, l, k, j, b);
    const double wn = std::sqrt(den / (l * j));
    std::cout << std::format("motor roots {:.2f} and {:.1f} 1/s (time constants {:.4f} s, {:.6f} s); wn = {:.1f}, zeta = {:.2f}\n",
                             m.slow, m.fast, -1 / m.slow, -1 / m.fast, wn, (r * j + l * b) / (l * j) / (2 * wn));
    for (double lx : {1e-3, 10e-3, 50e-3}) {
        double cur = 0.0;
        const double t = time63(lx, j, cur);
        const double wnx = std::sqrt(den / (lx * j));
        std::cout << std::format("L = {:.0f} mH: zeta = {:.2f}, 63 % time = {:.4f} s (tau_m = {:.4f})\n", lx * 1000,
                                 (r * j + lx * b) / (lx * j) / (2 * wnx), t, j * r / den);
    }
    double c1 = 0.0;
    double c2 = 0.0;
    const double t1 = time63(l, j, c1);
    const double t2 = time63(l, 2 * j, c2);
    std::cout << std::format("Q8: J doubled: 63 % time {:.4f} -> {:.4f} s; current at that row {:.3f} -> {:.3f} A\n", t1,
                             t2, c1, c2);
    auto mp = [&](double z) { return std::exp(-pi * z / std::sqrt(1 - z * z)); };
    std::cout << std::format("zeta 0.2: wd = {:.3f}, Mp = {:.3f}, tp = {:.3f}, ts = {:.1f}\n", 10 * std::sqrt(0.96),
                             mp(0.2), pi / (10 * std::sqrt(0.96)), 4 / (0.2 * 10));
    std::cout << std::format("zeta 0.3: Mp = {:.3f}, tp = {:.3f}; ln 50 = {:.2f}\n", mp(0.3), pi / (10 * std::sqrt(0.91)),
                             std::log(50.0));
    std::cout << std::format("Q1: {:.4f}; Q2: wn = {:.0f}, zeta = {:.2f}; Q4: Mp = {:.3f}, wd = {:.2f}, tp = {:.3f}, ts = {:.1f}\n",
                             25.3 / 40, std::sqrt(200 / 0.5), 4 / (2 * std::sqrt(200 * 0.5)), mp(0.5),
                             20 * std::sqrt(0.75), pi / (20 * std::sqrt(0.75)), 4 / (0.5 * 20));
    const double lq = std::log(0.30);
    const double zq = -lq / std::sqrt(pi * pi + lq * lq);
    std::cout << std::format("Q6: zeta = {:.3f}, wd = {:.2f}, wn = {:.2f}\n", zq, pi / 0.25,
                             pi / 0.25 / std::sqrt(1 - zq * zq));
    const double lf = std::log(0.4435);
    const double zf = -lf / std::sqrt(pi * pi + lf * lf);
    std::cout << std::format("forensic: zeta = {:.3f}, wd = {:.2f}, wn = {:.2f}; designed zeta 0.7: Mp = {:.3f}, tp = {:.3f} s\n",
                             zf, pi / 0.13, pi / 0.13 / std::sqrt(1 - zf * zf), mp(0.7), pi / (25 * std::sqrt(1 - 0.49)));
    std::cout << std::format("true forensic system: wn = 2 pi 4 = {:.2f} rad/s, Mp(0.25) = {:.3f}\n", 2 * pi * 4, mp(0.25));
    return 0;
}

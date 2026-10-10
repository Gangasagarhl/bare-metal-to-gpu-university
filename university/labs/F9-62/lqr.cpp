// lqr.cpp - an LQR for the cart-pole: weights from Bryson's rule, the gain, the closed-loop
// poles, and a balance test on the nonlinear cart-pole with a 10 N force limit.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "cartpole.hpp"
#include "lin.hpp"
#include "lqr.hpp"

int main()
{
    const CartPole p;
    const double T = 0.01, Fmax = 10.0;
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    // Bryson's rule: weight = 1 / (largest acceptable value)^2
    const Mat Q = diag({1 / (0.2 * 0.2), 1 / (1.0 * 1.0), 1 / (0.1 * 0.1), 1 / (1.0 * 1.0)});
    const Mat R = diag({1 / (Fmax * Fmax)});
    const LqrResult d = dlqr(Ad, Bd, Q, R);
    std::printf("Riccati iterations: %d\n", d.iters);
    printMat("K", d.K);
    printMat("P", d.P);
    const auto z = eig(Ad - Bd * d.K);
    printEig("closed-loop z", z);
    std::printf("as s = ln(z)/T:");
    for (const auto& zi : z) {
        const auto s = std::log(zi) / T;
        std::printf("  %.3f%+.3fj", s.real(), s.imag());
    }
    std::printf("\n");

    State s{0.0, 0.0, 0.05, 0.0};
    const Mat x0(4, 1, {s[0], s[1], s[2], s[3]});
    std::printf("predicted cost x0^T P x0 = %.4f\n", (tr(x0) * d.P * x0)(0, 0));
    double J = 0.0, peak = 0.0;
    std::printf("nonlinear cart-pole from th = 0.05 rad, |F| <= %.0f N\n", Fmax);
    std::printf("   t(s)     x(m)   th(rad)    F(N)\n");
    for (int k = 0; k <= 500; ++k) {
        const Mat x(4, 1, {s[0], s[1], s[2], s[3]});
        const double F = std::clamp(-(d.K * x)(0, 0), -Fmax, Fmax);
        J += (tr(x) * Q * x)(0, 0) + F * R(0, 0) * F;
        peak = std::fmax(peak, std::fabs(F));
        if (k % 50 == 0) std::printf("  %5.2f  %7.4f  %8.4f  %6.2f\n", k * T, s[0], s[2], F);
        for (int i = 0; i < 10; ++i) s = rk4(p, s, F, T / 10);
    }
    std::printf("measured cost over 5 s = %.4f, peak |F| = %.2f N\n", J, peak);
    return 0;
}

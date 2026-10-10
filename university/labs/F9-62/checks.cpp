// checks.cpp - recomputes the numbers quoted in the text of F9-62: the scalar worked example,
// the Lyapunov identity, gain and delay margins, and the delay-aware redesign.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <deque>
#include <numbers>
#include "cartpole.hpp"
#include "lin.hpp"
#include "lqr.hpp"

static double specRad(const Mat& M)
{
    double r = 0.0;
    for (const auto& z : eig(M)) r = std::fmax(r, std::abs(z));
    return r;
}

// Closed loop with a d-sample actuation delay: state (x, u[k-d], ..., u[k-1]).
static Mat delayed(const Mat& Ad, const Mat& Bd, const Mat& K, int d)
{
    if (d == 0) return Ad - Bd * K;
    const std::size_t n = 4 + static_cast<std::size_t>(d);
    Mat Z(n, n);
    for (std::size_t i = 0; i < 4; ++i) {
        for (std::size_t j = 0; j < 4; ++j) Z(i, j) = Ad(i, j);
        Z(i, 4) = Bd(i, 0);  // the oldest command is the one applied now
    }
    for (std::size_t k = 4; k + 1 < n; ++k) Z(k, k + 1) = 1.0;  // the queue moves up
    for (std::size_t j = 0; j < 4; ++j) Z(n - 1, j) = -K(0, j);  // the newest command
    return Z;
}

int main()
{
    // 1. Scalar worked example: a = 1.1, b = 1, q = 1, r = 1.
    const LqrResult s1 = dlqr(Mat(1, 1, {1.1}), Mat(1, 1, {1.0}), diag({1.0}), diag({1.0}));
    std::printf("scalar: p = %.5f (hand %.5f), K = %.5f, a - bK = %.5f\n", s1.P(0, 0),
                (1.21 + std::sqrt(1.21 * 1.21 + 4.0)) / 2.0, s1.K(0, 0), 1.1 - s1.K(0, 0));

    const CartPole p;
    const double T = 0.01;
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    const Mat Q = diag({25.0, 1.0, 100.0, 1.0});
    const Mat R = diag({0.01});
    const LqrResult d = dlqr(Ad, Bd, Q, R);
    const Mat Acl = Ad - Bd * d.K;

    // 2. Lyapunov identity: Acl^T P Acl - P = -(Q + K^T R K).
    const Mat resid = tr(Acl) * d.P * Acl - d.P + Q + tr(d.K) * R * d.K;
    std::printf("Lyapunov identity: largest residual %.1e, relative to largest P entry %.1e\n",
                maxAbs(resid), maxAbs(resid) / maxAbs(d.P));

    // 3. Gain margin: the range of factors kappa for which Ad - kappa Bd K stays stable.
    // bisection on each side of kappa = 1 (assumes one crossing on each side)
    auto stable = [&](double kappa) { return specRad(Ad - kappa * Bd * d.K) < 1.0; };
    double a = 0.0, c = 1.0;  // a: unstable side, c: stable side
    for (int it = 0; it < 40; ++it) (stable(0.5 * (a + c)) ? c : a) = 0.5 * (a + c);
    const double lo = c;
    a = 1.0;
    c = 100.0;  // a: stable side, c: unstable side
    for (int it = 0; it < 40; ++it) (stable(0.5 * (a + c)) ? a : c) = 0.5 * (a + c);
    const double hi = a;
    std::printf("gain margin: stable for %.3f < kappa < %.3f\n", lo, hi);

    // 4. Delay margin of four designs (R scaled): spectral radius for d = 0..6 samples.
    std::printf("spectral radius with a d-sample actuation delay (>= 1 means unstable)\n");
    std::printf("  R scale     d=0     d=1     d=2     d=3     d=4     d=5     d=6\n");
    for (double rho : {0.1, 1.0, 10.0, 100.0}) {
        const Mat K = dlqr(Ad, Bd, Q, diag({0.01 * rho})).K;
        std::printf("  %7.1f", rho);
        for (int dd = 0; dd <= 6; ++dd) std::printf("  %6.4f", specRad(delayed(Ad, Bd, K, dd)));
        std::printf("\n");
    }
    const auto zd = eig(delayed(Ad, Bd, d.K, 4));
    for (const auto& z : zd)
        if (std::abs(z) > 1.0 && z.imag() > 0.0)
            std::printf("design R = 0.01 with d = 4: unstable pair |z| = %.4f at %.2f Hz\n",
                        std::abs(z), std::arg(z) / (2.0 * std::numbers::pi * T));

    // 5. Delay-aware redesign: LQR on the model that includes the 4-sample queue.
    const int lag = 4;
    const std::size_t n = 4 + lag;
    Mat Aa(n, n), Ba(n, 1), Qa(n, n);
    for (std::size_t i = 0; i < 4; ++i) {
        for (std::size_t j = 0; j < 4; ++j) {
            Aa(i, j) = Ad(i, j);
            Qa(i, j) = Q(i, j);
        }
        Aa(i, 4) = Bd(i, 0);
    }
    for (std::size_t k = 4; k + 1 < n; ++k) Aa(k, k + 1) = 1.0;
    Ba(n - 1, 0) = 1.0;
    const LqrResult da = dlqr(Aa, Ba, Qa, R);
    std::printf("delay-aware design: closed-loop spectral radius = %.4f\n",
                specRad(Aa - Ba * da.K));
    // run it on the nonlinear robot with the queue and the 10 N limit
    State s{0.0, 0.0, 0.02, 0.0};
    std::deque<double> queue(lag, 0.0);
    double maxLate = 0.0;
    for (int k = 0; k <= 300; ++k) {
        double F = 0.0;
        for (std::size_t i = 0; i < 4; ++i) F -= da.K(0, i) * s[i];
        for (std::size_t i = 0; i < static_cast<std::size_t>(lag); ++i)
            F -= da.K(0, 4 + i) * queue[i];  // the controller knows what is still in flight
        F = std::clamp(F, -10.0, 10.0);
        queue.push_back(F);
        const double Fapp = queue.front();
        queue.pop_front();
        for (int i = 0; i < 10; ++i) s = rk4(p, s, Fapp, T / 10);
        if (k >= 100) maxLate = std::fmax(maxLate, std::fabs(s[2]));
    }
    std::printf("delay-aware design on the delayed robot: max |th| from 1 s to 3 s = %.5f rad\n",
                maxLate);
    return 0;
}

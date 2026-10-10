// buzz_log.cpp - evidence generator for the F9-63 forensic lab ("the cart that buzzes").
// Same robot, same LQR; only the observer's design weight W was changed.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include "cartpole.hpp"
#include "lin.hpp"
#include "lqr.hpp"
#include "noise.hpp"

struct Stats
{
    double rmsTh, rmsDF, peakF;
};

static Stats run(const CartPole& p, const Mat& Ad, const Mat& Bd, const Mat& K, const Mat& L,
                 bool print)
{
    const double T = 0.01, qx = 0.001, qth = 2.0 * std::numbers::pi / 2048.0;
    Noise rng(7);
    State s{0.0, 0.0, 0.05, 0.0};
    Mat xh(4, 1);
    double Fprev = 0.0, sth = 0.0, sdF = 0.0, peak = 0.0;
    int n = 0;
    for (int k = 0; k < 1000; ++k) {
        const long cx = std::lround(s[0] / qx);  // encoder counts
        const long cth = std::lround(s[2] / qth);
        const double F = std::clamp(-(K * xh)(0, 0), -10.0, 10.0);
        if (print && k >= 500 && k < 530)
            std::printf("  %5.2f  %8ld  %9ld  %8.4f  %9.3f  %7.2f\n",
                        k * T, cx, cth, xh(2, 0), xh(3, 0), F);
        if (k >= 200) {
            sth += s[2] * s[2];
            sdF += (F - Fprev) * (F - Fprev);
            peak = std::fmax(peak, std::fabs(F));
            ++n;
        }
        const Mat innov(2, 1, {cx * qx - xh(0, 0), cth * qth - xh(2, 0)});
        xh = Ad * xh + F * Bd + L * innov;
        const double push = 0.2 * rng.gauss();
        for (int i = 0; i < 10; ++i) s = rk4(p, s, F + push, T / 10);
        Fprev = F;
    }
    return {1000.0 * std::sqrt(sth / n), std::sqrt(sdF / n), peak};
}

int main()
{
    const CartPole p;
    const double T = 0.01, qx = 0.001, qth = 2.0 * std::numbers::pi / 2048.0;
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    const Mat C(2, 4, {1, 0, 0, 0, 0, 0, 1, 0});
    const Mat K = dlqr(Ad, Bd, diag({25.0, 1.0, 100.0, 1.0}), diag({0.01})).K;
    const Mat V = diag({qx * qx / 12.0, qth * qth / 12.0});
    std::printf("encoders: cart 1 mm/count, pole 2048 counts/turn; controller 100 Hz\n");
    std::printf("config        observer |z| (from the boot log)            rms th(mrad)  rms dF(N)"
                "  peak|F|(N)\n");
    Mat Lnew;
    for (double sig : {0.2, 20.0}) {  // design weight W = sig^2 Bd Bd^T
        const Mat W = (sig * sig) * (Bd * tr(Bd)) + 1e-10 * eye(4);
        const Mat L = tr(dlqr(tr(Ad), tr(C), W, V).K);
        const auto z = eig(Ad - L * C);
        const Stats st = run(p, Ad, Bd, K, L, false);
        std::printf("%-12s ", sig < 1.0 ? "last week" : "this week");
        for (const auto& zi : z) std::printf(" %.3f", std::abs(zi));
        std::printf("   %12.3f  %9.3f  %10.2f\n", st.rmsTh, st.rmsDF, st.peakF);
        Lnew = L;
    }
    printMat("this week's L", Lnew);
    std::printf("this week, 5.00-5.29 s:\n");
    std::printf("  t(s)   x_count  th_count   th_est   w_est      F(N)\n");
    run(p, Ad, Bd, K, Lnew, true);
    return 0;
}

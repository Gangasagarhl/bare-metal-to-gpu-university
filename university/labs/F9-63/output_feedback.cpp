// output_feedback.cpp - three ways to get the velocities the LQR needs, with encoders that
// report whole counts and a random push on the cart: (a) cheat and use the true state,
// (b) finite differences of the counts, (c) a steady-state Kalman-designed observer.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include "cartpole.hpp"
#include "lin.hpp"
#include "lqr.hpp"
#include "noise.hpp"

int main()
{
    const CartPole p;
    const double T = 0.01, Fmax = 10.0;
    const double qx = 0.001;                              // cart encoder: 1 mm per count
    const double qth = 2.0 * std::numbers::pi / 2048.0;   // pole encoder: 2048 counts per turn
    const double sigF = 0.2;                              // random push on the cart, N (1 sigma)
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    const Mat C(2, 4, {1, 0, 0, 0,
                       0, 0, 1, 0});
    const Mat K = dlqr(Ad, Bd, diag({25.0, 1.0, 100.0, 1.0}), diag({0.01})).K;
    // Observer gain from the dual Riccati equation (a steady-state Kalman predictor):
    // process noise W = Bd Bd^T sigF^2 (+ a little on every state), sensor noise V = q^2/12.
    const Mat W = (sigF * sigF) * (Bd * tr(Bd)) + 1e-10 * eye(4);
    const Mat V = diag({qx * qx / 12.0, qth * qth / 12.0});
    const Mat L = tr(dlqr(tr(Ad), tr(C), W, V).K);
    printMat("L (Kalman-designed observer gain)", L);
    printEig("eig(Ad - L C)", eig(Ad - L * C));

    std::printf("10 s run from th = 0.05 rad; statistics over 2-10 s\n");
    std::printf("  method                rms th (mrad)  rms x (mm)  rms dF (N)  peak |F| (N)\n");
    const char* names[3] = {"(a) true state", "(b) finite differences", "(c) observer"};
    for (int method = 0; method < 3; ++method) {
        Noise rng(2026);  // the same pushes for every method
        State s{0.0, 0.0, 0.05, 0.0};
        Mat xh(4, 1);
        double yxPrev = 0.0, ythPrev = quantise(0.05, qth);
        double Fprev = 0.0, sth = 0.0, sx = 0.0, sdF = 0.0, peak = 0.0;
        int count = 0;
        for (int k = 0; k < 1000; ++k) {
            const double yx = quantise(s[0], qx);
            const double yth = quantise(s[2], qth);
            Mat xu(4, 1);
            if (method == 0) xu = Mat(4, 1, {s[0], s[1], s[2], s[3]});
            if (method == 1) xu = Mat(4, 1, {yx, (yx - yxPrev) / T, yth, (yth - ythPrev) / T});
            if (method == 2) xu = xh;
            const double F = std::clamp(-(K * xu)(0, 0), -Fmax, Fmax);
            if (k >= 200) {
                sth += s[2] * s[2];
                sx += s[0] * s[0];
                sdF += (F - Fprev) * (F - Fprev);
                peak = std::fmax(peak, std::fabs(F));
                ++count;
            }
            const Mat innov(2, 1, {yx - xh(0, 0), yth - xh(2, 0)});
            xh = Ad * xh + F * Bd + L * innov;
            const double push = sigF * rng.gauss();
            for (int i = 0; i < 10; ++i) s = rk4(p, s, F + push, T / 10);
            yxPrev = yx;
            ythPrev = yth;
            Fprev = F;
        }
        std::printf("  %-22s %13.3f  %10.3f  %10.3f  %12.2f\n", names[method],
                    1000.0 * std::sqrt(sth / count), 1000.0 * std::sqrt(sx / count),
                    std::sqrt(sdF / count), peak);
    }
    return 0;
}

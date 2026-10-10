// two_poles.cpp - one cart, two poles side by side. How hard is it to steer both poles with
// one force as the lengths become equal? Minimum-energy input over 1 s, sampled at 100 Hz.
#include <cmath>
#include <cstdio>
#include "lin.hpp"

// State (x, v, th1, w1, th2, w2); linearised about both poles upright (derived in Layer 2).
static Mat twoA(double M, double m1, double m2, double l1, double l2, double b, double g)
{
    Mat A(6, 6);
    A(0, 1) = 1.0;
    A(1, 1) = -b / M;
    A(1, 2) = -m1 * g / M;
    A(1, 4) = -m2 * g / M;
    A(2, 3) = 1.0;
    A(4, 5) = 1.0;
    const double L[2] = {l1, l2};
    for (int i = 0; i < 2; ++i) {  // l_i th_i'' = g th_i - x''
        const std::size_t r = 3 + 2 * static_cast<std::size_t>(i);
        A(r, 1) = b / (M * L[i]);
        A(r, 2) = m1 * g / (M * L[i]);
        A(r, 4) = m2 * g / (M * L[i]);
        A(r, r - 1) += g / L[i];
    }
    return A;
}

static Mat twoB(double M, double l1, double l2)
{
    return Mat(6, 1, {0, 1.0 / M, 0, -1.0 / (M * l1), 0, -1.0 / (M * l2)});
}

int main()
{
    const double M = 1.0, m1 = 0.2, m2 = 0.2, b = 0.1, g = 9.81, l1 = 0.5;
    const double T = 0.01;
    const int N = 100;  // 1 s of samples
    const Mat target(6, 1, {0, 0, 0.05, 0, -0.05, 0});  // poles leaning apart, cart back at 0
    std::printf("goal: from rest upright to th1 = +0.05, th2 = -0.05 rad in %d steps of %.2f s\n",
                N, T);
    std::printf("   l2(m)  rank of [B AB ... A^5B]   peak |F| (N)   sum F^2 T (N^2 s)\n");
    for (double l2 : {0.25, 0.40, 0.45, 0.49, 0.499, 0.50}) {
        const Mat A = twoA(M, m1, m2, l1, l2, b, g);
        const Mat B = twoB(M, l1, l2);
        // continuous-time controllability matrix [B AB ... A^5 B]
        Mat Wc(6, 6);
        Mat blk = B;
        for (std::size_t k = 0; k < 6; ++k) {
            for (std::size_t i = 0; i < 6; ++i) Wc(i, k) = blk(i, 0);
            blk = A * blk;
        }
        std::printf("  %6.3f  %21zu", l2, rank(Wc));
        // reachability over N steps: x_N = R u with R = [Ad^(N-1) Bd, ..., Ad Bd, Bd]
        const auto [Ad, Bd] = c2d(A, B, T);
        Mat R(6, N);
        Mat col = Bd;
        for (int k = N - 1; k >= 0; --k) {
            for (std::size_t i = 0; i < 6; ++i) R(i, static_cast<std::size_t>(k)) = col(i, 0);
            col = Ad * col;
        }
        try {
            const Mat u = tr(R) * inv(R * tr(R)) * target;  // minimum-norm input sequence
            double peak = 0.0, energy = 0.0;
            for (double v : u.a) {
                peak = std::fmax(peak, std::fabs(v));
                energy += v * v * T;
            }
            std::printf("  %13.3g  %18.3g\n", peak, energy);
        } catch (const std::exception& e) {
            std::printf("  no input reaches the goal (%s)\n", e.what());
        }
    }
    return 0;
}

// rig_log.cpp - evidence generator for the F9-60 forensic lab ("the double-pole rig that
// would not move"). Same linearised model as two_poles.cpp, with the rig's lengths.
#include <cmath>
#include <cstdio>
#include "lin.hpp"

int main()
{
    const double M = 1.0, m = 0.2, b = 0.1, g = 9.81, l1 = 0.50, l2 = 0.48, T = 0.01;
    const int N = 100;
    Mat A(6, 6);
    A(0, 1) = 1.0;
    A(1, 1) = -b / M;
    A(1, 2) = -m * g / M;
    A(1, 4) = -m * g / M;
    A(2, 3) = 1.0;
    A(4, 5) = 1.0;
    const double L[2] = {l1, l2};
    for (std::size_t i = 0; i < 2; ++i) {
        const std::size_t r = 3 + 2 * i;
        A(r, 1) = b / (M * L[i]);
        A(r, 2) = m * g / (M * L[i]);
        A(r, 4) = m * g / (M * L[i]);
        A(r, r - 1) += g / L[i];
    }
    const Mat B(6, 1, {0, 1.0 / M, 0, -1.0 / (M * l1), 0, -1.0 / (M * l2)});
    const auto [Ad, Bd] = c2d(A, B, T);
    Mat R(6, N);
    Mat col = Bd;
    for (int k = N - 1; k >= 0; --k) {
        for (std::size_t i = 0; i < 6; ++i) R(i, static_cast<std::size_t>(k)) = col(i, 0);
        col = Ad * col;
    }
    Mat Wc(6, 6);
    Mat blk = B;
    for (std::size_t k = 0; k < 6; ++k) {
        for (std::size_t i = 0; i < 6; ++i) Wc(i, k) = blk(i, 0);
        blk = A * blk;
    }
    std::printf("rig: M = %.1f kg, two poles m = %.1f kg, l1 = %.2f m, l2 = %.2f m\n",
                M, m, l1, l2);
    std::printf("design check: rank [B AB ... A^5 B] = %zu of 6  -> 'controllable'\n", rank(Wc));
    printEig("open-loop poles", eig(A));
    const auto gram = symEig(R * tr(R));
    std::printf("1-s reachability Gram R R^T, eigenvalues:");
    for (double v : gram) std::printf(" %.3g", v);
    std::printf("\n");
    std::printf("minimum-energy moves over 1 s (rest -> goal):\n");
    std::printf("  goal (x, v, th1, w1, th2, w2)             peak |F| (N)\n");
    const double goals[4][6] = {{0.02, 0, 0, 0, 0, 0},
                                {0, 0, 0.01, 0, 0.01, 0},
                                {0, 0, 0.01, 0, 0.00, 0},
                                {0, 0, 0.01, 0, -0.01, 0}};
    for (const auto& gl : goals) {
        const Mat x(6, 1, {gl[0], gl[1], gl[2], gl[3], gl[4], gl[5]});
        const Mat u = tr(R) * inv(R * tr(R)) * x;
        double peak = 0.0;
        for (double v : u.a) peak = std::fmax(peak, std::fabs(v));
        std::printf("  (%5.2f, %g, %5.2f, %g, %5.2f, %g)            %12.1f\n",
                    gl[0], gl[1], gl[2], gl[3], gl[4], gl[5], peak);
    }
    std::printf("motor driver limit: 20 N\n");
    return 0;
}

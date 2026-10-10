// wall_log.cpp - evidence generator for the F9-64 forensic lab ("the cart that hit the wall").
// The same MPC as mpc.cpp; only the horizon was shortened to save computing time.
#include <cmath>
#include <cstdio>
#include "lin.hpp"
#include "lqr.hpp"
#include "mpc.hpp"

int main()
{
    const double M = 1.0, b = 0.1, T = 0.05, Fmax = 2.0, target = 1.0, wall = 1.1;
    const auto [Ad, Bd] = c2d(Mat(2, 2, {0, 1, 0, -b / M}), Mat(2, 1, {0, 1.0 / M}), T);
    const Mat Q = diag({100.0, 1.0});
    const double R = 0.01;
    const LqrResult d = dlqr(Ad, Bd, Q, diag({R}));
    for (std::size_t N : {20u, 5u}) {
        Mpc mpc = makeMpc(Ad, Bd, Q, R, d.P, N, Fmax, wall - target, 1e3);
        Mat x(2, 1, {0.0, 0.0});
        double xmax = 0.0;
        std::printf("config: T = %.2f s, |F| <= %.0f N, wall %.1f m, horizon N = %zu (%.2f s)\n",
                    T, Fmax, wall, N, static_cast<double>(N) * T);
        if (N == 5)
            std::printf("  t(s)    x(m)   v(m/s)   F(N)   predicted x at horizon end"
                        "   max predicted x\n");
        for (int k = 0; k <= 60; ++k) {
            const Mat e(2, 1, {x(0, 0) - target, x(1, 0)});
            const auto U = solve(mpc, e, 300);
            // the plan's predicted positions x_1 .. x_N (back in track coordinates)
            const Mat X = mpc.Sx * e + mpc.Su * Mat(N, 1, U);
            double pmax = -1e9;
            for (std::size_t j = 0; j < N; ++j) pmax = std::fmax(pmax, X(2 * j, 0) + target);
            if (N == 5 && k % 2 == 0 && k <= 40)
                std::printf(" %5.2f  %6.3f  %6.3f  %5.2f  %27.3f  %16.3f\n", k * T, x(0, 0),
                            x(1, 0), U[0], X(2 * (N - 1), 0) + target, pmax);
            x = Ad * x + U[0] * Bd;
            xmax = std::fmax(xmax, x(0, 0));
        }
        std::printf("  furthest x reached: %.3f m\n", xmax);
    }
    return 0;
}

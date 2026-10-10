// mpc.cpp - park a cart 1.0 m away, next to a wall at 1.1 m, with a 2 N force limit.
// Compare an LQR whose output is clipped to the limit with an MPC that plans with the limit.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "lin.hpp"
#include "lqr.hpp"
#include "mpc.hpp"

int main()
{
    const double M = 1.0, b = 0.1, T = 0.05, Fmax = 2.0, target = 1.0, wall = 1.1;
    const Mat A(2, 2, {0, 1, 0, -b / M});
    const Mat B(2, 1, {0, 1.0 / M});
    const auto [Ad, Bd] = c2d(A, B, T);
    const Mat Q = diag({100.0, 1.0});
    const double R = 0.01;
    const LqrResult d = dlqr(Ad, Bd, Q, diag({R}));
    printMat("LQR K", d.K);
    Mpc mpc = makeMpc(Ad, Bd, Q, R, d.P, 40, Fmax, wall - target, 1e3);  // works in error coords
    std::printf("MPC: horizon 40 steps = %.1f s, solver 300 iterations per step\n", 40 * T);

    for (int method = 0; method < 2; ++method) {
        Mat x(2, 1, {0.0, 0.0});  // position, velocity (true cart)
        double xmax = 0.0, J = 0.0, tArrive = -1.0;
        std::printf("%s\n   t(s)   x(m)    v(m/s)   F(N)\n", method == 0 ? "clipped LQR" : "MPC");
        for (int k = 0; k <= 100; ++k) {
            const Mat e(2, 1, {x(0, 0) - target, x(1, 0)});  // error from the parking spot
            double F = 0.0;
            if (method == 0) F = std::clamp(-(d.K * e)(0, 0), -Fmax, Fmax);
            else F = solve(mpc, e, 300)[0];
            J += (tr(e) * Q * e)(0, 0) + R * F * F;
            if (k % 5 == 0)
                std::printf("  %5.2f  %6.3f  %7.3f  %6.2f\n", k * T, x(0, 0), x(1, 0), F);
            x = Ad * x + F * Bd;
            xmax = std::fmax(xmax, x(0, 0));
            if (tArrive < 0.0 && std::fabs(x(0, 0) - target) < 0.01 && std::fabs(x(1, 0)) < 0.02)
                tArrive = (k + 1) * T;
        }
        std::printf("  furthest x = %.3f m (wall at %.1f m), parked (1 cm, 2 cm/s) at t = %.2f s,"
                    " cost = %.2f\n", xmax, wall, tArrive, J);
    }
    return 0;
}

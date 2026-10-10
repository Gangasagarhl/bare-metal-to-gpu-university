// checks.cpp - recomputes the numbers quoted in the text of F9-64.
#include <cmath>
#include <cstdio>
#include "lin.hpp"
#include "lqr.hpp"
#include "mpc.hpp"

int main()
{
    // 1. Worked example: x+ = x + u, stage cost x^2 + u^2, terminal weight 1, x0 = 3.
    const Mat A(1, 1, {1.0});
    const Mat B(1, 1, {1.0});
    const Mat Q = diag({1.0});
    for (double umax : {5.0, 1.0}) {
        Mpc one = makeMpc(A, B, Q, 1.0, Q, 1, umax, 1e9, 0.0);
        Mpc two = makeMpc(A, B, Q, 1.0, Q, 2, umax, 1e9, 0.0);
        const Mat x0(1, 1, {3.0});
        const auto u1 = solve(one, x0, 2000);
        const auto u2 = solve(two, x0, 2000);
        std::printf("|u| <= %.0f: N = 1 -> u0 = %.4f;  N = 2 -> u0 = %.4f, u1 = %.4f\n",
                    umax, u1[0], u2[0], u2[1]);
    }

    // 2. With no limit active, the MPC's first move equals the LQR move -K e.
    const double T = 0.05;
    const auto [Ad, Bd] = c2d(Mat(2, 2, {0, 1, 0, -0.1}), Mat(2, 1, {0, 1.0}), T);
    const Mat Qc = diag({100.0, 1.0});
    const LqrResult d = dlqr(Ad, Bd, Qc, diag({0.01}));
    const Mat e(2, 1, {-0.01, 0.0});
    std::printf("small error (-0.01 m): LQR -K e = %.5f N\n", -(d.K * e)(0, 0));
    struct Case { double rho; int iters; };
    for (const Case c : {Case{0.0, 20000}, Case{1e3, 300}, Case{1e3, 3000}, Case{1e3, 30000}}) {
        Mpc m = makeMpc(Ad, Bd, Qc, 0.01, d.P, 40, 2.0, 0.1, c.rho);
        std::printf("  MPC first move, wall weight %4.0f, %5d iterations, cold start: %.5f N\n",
                    c.rho, c.iters, solve(m, e, c.iters)[0]);
    }

    // 3. Work per control step for the teaching solver (multiply-adds in H U only).
    const double macs = 300.0 * 40.0 * 40.0;
    std::printf("solver work per step: 300 iterations x 40 x 40 = %.0f multiply-adds (H U only),"
                " every %.2f s\n", macs, T);
    std::printf("horizon 5: 300 x 5 x 5 = %.0f multiply-adds\n", 300.0 * 5.0 * 5.0);

    // 4. Horizon sweep for the parking task (as wall_log.cpp): furthest position reached.
    for (std::size_t N : {5u, 10u, 15u, 20u}) {
        Mpc m = makeMpc(Ad, Bd, Qc, 0.01, d.P, N, 2.0, 0.1, 1e3);
        Mat x(2, 1, {0.0, 0.0});
        double xmax = 0.0, vmax = 0.0;
        for (int k = 0; k <= 60; ++k) {
            const Mat ek(2, 1, {x(0, 0) - 1.0, x(1, 0)});
            x = Ad * x + solve(m, ek, 300)[0] * Bd;
            xmax = std::fmax(xmax, x(0, 0));
            vmax = std::fmax(vmax, x(1, 0));
        }
        std::printf("horizon %2zu (%.2f s): top speed %.3f m/s, furthest x %.3f m (wall 1.1 m)\n",
                    N, static_cast<double>(N) * T, vmax, xmax);
    }
    return 0;
}

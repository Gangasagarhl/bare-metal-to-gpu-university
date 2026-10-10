// qr_sweep.cpp - the LQR trade-off: make force cheaper or dearer (scale R) and watch the gain,
// the slowest closed-loop pole, the peak force and the settling time change.
#include <cmath>
#include <complex>
#include <cstdio>
#include "cartpole.hpp"
#include "lin.hpp"
#include "lqr.hpp"

int main()
{
    const CartPole p;
    const double T = 0.01;
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    const Mat Q = diag({25.0, 1.0, 100.0, 1.0});
    std::printf("start th = 0.05 rad, no force limit; settled = |x| < 1 cm and |th| < 0.005 rad\n");
    std::printf("  R scale  K_x      K_v      K_th      K_w     slowest s (1/s)  peak|F|(N)"
                "  settle(s)\n");
    for (double rho : {0.01, 0.1, 1.0, 10.0, 100.0}) {
        const Mat R = diag({0.01 * rho});
        const LqrResult d = dlqr(Ad, Bd, Q, R);
        double slow = -1e9;
        for (const auto& z : eig(Ad - Bd * d.K)) slow = std::fmax(slow, (std::log(z) / T).real());
        State s{0.0, 0.0, 0.05, 0.0};
        double peak = 0.0, settle = 0.0;
        for (int k = 0; k < 1500; ++k) {
            double F = 0.0;
            for (std::size_t i = 0; i < 4; ++i) F -= d.K(0, i) * s[i];
            peak = std::fmax(peak, std::fabs(F));
            if (std::fabs(s[0]) >= 0.01 || std::fabs(s[2]) >= 0.005) settle = (k + 1) * T;
            for (int i = 0; i < 10; ++i) s = rk4(p, s, F, T / 10);
        }
        std::printf("  %7.2f  %7.2f  %7.2f  %8.2f  %7.2f  %15.3f  %10.2f  %9.2f\n", rho,
                    d.K(0, 0), d.K(0, 1), d.K(0, 2), d.K(0, 3), slow, peak, settle);
    }
    return 0;
}

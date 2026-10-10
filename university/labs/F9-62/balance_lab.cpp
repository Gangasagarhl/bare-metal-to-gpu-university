// balance_lab.cpp - the RB402 course lab: balance the simulated cart-pole with LQR and find,
// for each design, the largest starting lean it can still catch under the rig's limits.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include "cartpole.hpp"
#include "lin.hpp"
#include "lqr.hpp"

struct Limits
{
    double Fmax = 10.0;   // actuator limit, N
    double track = 0.5;   // cart must stay within +/- this, m
};

// Acceptance test of one run: never off the track, pole never past 1 rad,
// and after 5 s both |x| < 2 cm and |th| < 0.01 rad.
static bool caught(const CartPole& p, const Mat& K, double T, const Limits& lim, double th0)
{
    State s{0.0, 0.0, th0, 0.0};
    for (int k = 0; k < static_cast<int>(5.0 / T); ++k) {
        double F = 0.0;
        for (std::size_t i = 0; i < 4; ++i) F -= K(0, i) * s[i];
        F = std::clamp(F, -lim.Fmax, lim.Fmax);
        for (int i = 0; i < 10; ++i) s = rk4(p, s, F, T / 10);
        if (std::fabs(s[0]) > lim.track || std::fabs(s[2]) > 1.0) return false;
    }
    return std::fabs(s[0]) < 0.02 && std::fabs(s[2]) < 0.01;
}

int main()
{
    const CartPole p;
    const Limits lim;
    const double T = 0.01;
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    const Mat Q = diag({25.0, 1.0, 100.0, 1.0});
    std::printf("limits: |F| <= %.0f N, |x| <= %.1f m, controller at %.0f Hz\n",
                lim.Fmax, lim.track, 1.0 / T);
    std::printf("  R scale   K_th     largest caught lean (rad)   (deg)\n");
    for (double rho : {0.1, 1.0, 10.0, 100.0}) {
        const Mat K = dlqr(Ad, Bd, Q, diag({0.01 * rho})).K;
        double lo = 0.0, hi = 1.2;  // bisection: lo is caught, hi is not
        if (!caught(p, K, T, lim, 0.01)) {
            std::printf("  %7.1f  %7.1f   fails even at 0.01 rad\n", rho, K(0, 2));
            continue;
        }
        lo = 0.01;
        for (int it = 0; it < 30; ++it) {
            const double mid = 0.5 * (lo + hi);
            (caught(p, K, T, lim, mid) ? lo : hi) = mid;
        }
        std::printf("  %7.1f  %7.1f   %25.3f   %5.1f\n",
                    rho, K(0, 2), lo, lo * 180.0 / std::numbers::pi);
    }
    return 0;
}

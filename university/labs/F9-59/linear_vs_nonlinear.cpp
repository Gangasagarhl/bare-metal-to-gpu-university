// linear_vs_nonlinear.cpp - how far from upright is the linear model still good?
// The pole is released at rest from a small and a larger lean, with no force on the cart.
#include <cmath>
#include <cstdio>
#include <numbers>
#include "cartpole.hpp"
#include "lin.hpp"

int main()
{
    const CartPole p;
    const double T = 0.01;
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    for (double th0 : {0.05, 0.30}) {
        State s{0.0, 0.0, th0, 0.0};   // nonlinear "robot", stepped by RK4
        Mat x(4, 1, {0.0, 0.0, th0, 0.0});  // linear model, stepped exactly
        std::printf("released from th0 = %.2f rad (%.1f deg)\n",
                    th0, th0 * 180.0 / std::numbers::pi);
        std::printf("   t(s)  th_nonlinear  th_linear  error(%%)    x_nonlin   x_linear\n");
        for (int k = 0; k <= 60; ++k) {
            if (k % 10 == 0) {
                const double err = 100.0 * (x(2, 0) - s[2]) / s[2];
                std::printf("  %5.2f  %12.4f  %9.4f  %8.1f  %10.4f  %9.4f\n",
                            k * T, s[2], x(2, 0), err, s[0], x(0, 0));
            }
            for (int i = 0; i < 10; ++i) s = rk4(p, s, 0.0, T / 10);
            x = Ad * x;  // F = 0, so the Bd term is zero
        }
    }
    return 0;
}

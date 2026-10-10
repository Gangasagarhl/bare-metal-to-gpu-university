// observer.cpp - a Luenberger observer for the cart-pole that sees only the cart position.
// The pole is balanced by an LQR acting on the ESTIMATED state (output feedback).
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>
#include "cartpole.hpp"
#include "lin.hpp"
#include "lqr.hpp"
#include "place.hpp"

using cd = std::complex<double>;

int main()
{
    const CartPole p;
    const double T = 0.01;
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    const Mat C(1, 4, {1, 0, 0, 0});  // the only sensor: cart position
    const Mat K = dlqr(Ad, Bd, diag({25.0, 1.0, 100.0, 1.0}), diag({0.01})).K;

    // Observer poles: four real poles, faster than the controller's (-1.7 ... -21 1/s).
    std::vector<cd> zo;
    for (double s : {-8.0, -9.0, -10.0, -11.0}) zo.push_back(std::exp(s * T));
    const Mat L = tr(acker(tr(Ad), tr(C), zo));  // duality: place eig(Ad^T - C^T L^T)
    printMat("L (observer gain)", L);
    printEig("eig(Ad - L C)", eig(Ad - L * C));

    State s{0.0, 0.0, 0.05, 0.0};  // the real cart-pole starts leaning 0.05 rad
    Mat xh(4, 1);                  // the observer starts believing everything is zero
    std::printf("   t(s)   th_true   th_est    w_true    w_est    |error|     F(N)\n");
    for (int k = 0; k <= 300; ++k) {
        const double F = std::clamp(-(K * xh)(0, 0), -10.0, 10.0);  // u = -K x_hat
        const double y = s[0];                                       // measure
        if (k % 20 == 0) {
            const Mat x(4, 1, {s[0], s[1], s[2], s[3]});
            const Mat e = x - xh;
            double ne = 0.0;
            for (double v : e.a) ne += v * v;
            std::printf("  %5.2f  %8.4f  %8.4f  %8.4f  %8.4f  %9.2e  %7.2f\n",
                        k * T, s[2], xh(2, 0), s[3], xh(3, 0), std::sqrt(ne), F);
        }
        // predict with the model, correct with the measurement surprise y - C x_hat
        xh = Ad * xh + F * Bd + (y - (C * xh)(0, 0)) * L;
        for (int i = 0; i < 10; ++i) s = rk4(p, s, F, T / 10);
    }
    return 0;
}

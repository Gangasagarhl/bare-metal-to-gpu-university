// checks.cpp - recomputes the numbers quoted in the text of F9-61.
#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>
#include "cartpole.hpp"
#include "lin.hpp"
#include "place.hpp"

using cd = std::complex<double>;

int main()
{
    // 1. Worked example: the F0-67 cart, poles at -2 +/- 2j, by Ackermann and by hand.
    const Mat A(2, 2, {0, 1, 0, -0.4});
    const Mat B(2, 1, {0, 0.5});
    const Mat K = acker(A, B, {cd(-2, 2), cd(-2, -2)});
    std::printf("cart: K = [%.4f, %.4f]  (hand: k1 = 8/0.5 = 16, k2 = (4 - 0.4)/0.5 = 7.2)\n",
                K(0, 0), K(0, 1));
    printEig("cart: eig(A - B K)", eig(A - B * K));

    // 2. The cart-pole design: desired polynomial and the achieved one.
    const CartPole p;
    const Mat Ac = cartA(p);
    const Mat Bc = cartB(p);
    const std::vector<cd> want{cd(-2, 1), cd(-2, -1), -5.0, -6.0};
    const auto d = polyFromRoots(want);
    std::printf("desired: s^4 + %.4f s^3 + %.4f s^2 + %.4f s + %.4f\n", d[1], d[2], d[3], d[4]);
    const auto c = charpoly(Ac - Bc * acker(Ac, Bc, want));
    std::printf("achieved: s^4 + %.4f s^3 + %.4f s^2 + %.4f s + %.4f\n", c[1], c[2], c[3], c[4]);

    // 3. Tracking a cart-position setpoint r = 0.2 m with u = -K (x - r e1).
    const double T = 0.01;
    const auto [Ad, Bd] = c2d(Ac, Bc, T);
    std::vector<cd> z;
    for (const auto& s : want) z.push_back(std::exp(s * T));
    const Mat Kd = acker(Ad, Bd, z);
    State s{0.0, 0.0, 0.0, 0.0};
    double xmin = 0.0;
    for (int k = 0; k < 800; ++k) {
        const State e{s[0] - 0.2, s[1], s[2], s[3]};
        double F = 0.0;
        for (std::size_t i = 0; i < 4; ++i) F -= Kd(0, i) * e[i];
        for (int i = 0; i < 10; ++i) s = rk4(p, s, F, T / 10);
        xmin = std::fmin(xmin, s[0]);
    }
    std::printf("setpoint 0.2 m: x after 8 s = %.4f m, lowest x on the way = %.4f m\n", s[0], xmin);

    // 4. Gain sizes for the speed sweep (how K grows with pole speed).
    for (double alpha : {1.0, 2.0, 3.0}) {
        std::vector<cd> za;
        for (const auto& w : want) za.push_back(std::exp(alpha * w * T));
        const Mat Ka = acker(Ad, Bd, za);
        std::printf("speed %.0fx: Kd = [%.1f, %.1f, %.1f, %.1f]\n",
                    alpha, Ka(0, 0), Ka(0, 1), Ka(0, 2), Ka(0, 3));
    }
    return 0;
}

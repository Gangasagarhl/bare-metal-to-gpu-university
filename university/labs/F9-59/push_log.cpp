// push_log.cpp - evidence generator for the F9-59 forensic lab ("the model that pushed the
// wrong way"). The "robot" is the nonlinear cart-pole; the "model" is the team's linear model.
#include <cstdio>
#include "cartpole.hpp"
#include "lin.hpp"

int main()
{
    const CartPole p;
    const double T = 0.01;
    Mat A = cartA(p);
    Mat B = cartB(p);
    B(3, 0) = +1.0 / (p.M * p.l);  // the team's model (fault injected here; see the answer key)
    const auto [Ad, Bd] = c2d(A, B, T);
    State s{0.0, 0.0, 0.0, 0.0};
    Mat x(4, 1);
    std::printf("push test: +2 N on the cart for 0.10 s, pole held upright at t = 0 then let go\n");
    std::printf("  t(s)   F(N)   x_robot  th_robot   x_model  th_model\n");
    for (int k = 0; k <= 30; ++k) {
        const double F = (k < 10) ? 2.0 : 0.0;
        if (k % 3 == 0)
            std::printf(" %5.2f  %5.1f  %8.4f  %8.4f  %8.4f  %8.4f\n",
                        k * T, F, s[0], s[2], x(0, 0), x(2, 0));
        for (int i = 0; i < 10; ++i) s = rk4(p, s, F, T / 10);
        x = Ad * x + F * Bd;
    }
    return 0;
}

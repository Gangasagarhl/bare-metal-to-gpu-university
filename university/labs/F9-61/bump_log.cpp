// bump_log.cpp - evidence generator for the F9-61 forensic lab ("the snappier controller
// that dropped the pole"). Nonlinear cart-pole, discrete pole placement, 10 N actuator.
#include <algorithm>
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
    const CartPole p;
    const double T = 0.01, Fmax = 10.0;
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    const std::vector<cd> poles{cd(-6, 3), cd(-6, -3), -15.0, -18.0};  // the "snappier" design
    std::vector<cd> z;
    for (const auto& s : poles) z.push_back(std::exp(s * T));
    const Mat K = acker(Ad, Bd, z);
    printMat("K in use", K);
    std::printf("bump test: pole released at th = 0.15 rad; driver clamps |F| to %.0f N\n", Fmax);
    std::printf("  t(s)     x(m)    v(m/s)   th(rad)  w(rad/s)  F_demand(N)  F_applied(N)\n");
    State s{0.0, 0.0, 0.15, 0.0};
    for (int k = 0; k <= 150 && std::fabs(s[2]) < 1.2; ++k) {
        double F = 0.0;
        for (std::size_t i = 0; i < 4; ++i) F -= K(0, i) * s[i];
        const double Fa = std::clamp(F, -Fmax, Fmax);
        if (k % 5 == 0)
            std::printf(" %5.2f  %7.3f  %8.3f  %8.3f  %8.3f  %11.1f  %12.1f\n",
                        k * T, s[0], s[1], s[2], s[3], F, Fa);
        for (int i = 0; i < 10; ++i) s = rk4(p, s, Fa, T / 10);
    }
    return 0;
}

// sat_sweep.cpp - faster poles need more force. With a 10 N actuator limit, how fast can the
// poles be before a 0.15 rad lean can no longer be caught?
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
    const double T = 0.01, Fmax = 10.0, th0 = 0.15;
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    const std::vector<cd> base{cd(-2, 1), cd(-2, -1), -5.0, -6.0};
    std::printf("start: th = %.2f rad, force limit %.0f N, controller at %.0f Hz\n",
                th0, Fmax, 1.0 / T);
    std::printf("  speed  poles (1/s)                 peak demand(N)  time at limit(s)  result\n");
    for (double alpha : {0.5, 1.0, 1.5, 2.0, 3.0, 4.0}) {
        std::vector<cd> z;
        for (const auto& s : base) z.push_back(std::exp(alpha * s * T));
        const Mat K = acker(Ad, Bd, z);
        State s{0.0, 0.0, th0, 0.0};
        double peak = 0.0, sat = 0.0;
        bool fell = false;
        for (int k = 0; k < 600 && !fell; ++k) {
            double F = 0.0;
            for (std::size_t i = 0; i < 4; ++i) F -= K(0, i) * s[i];
            peak = std::fmax(peak, std::fabs(F));
            if (std::fabs(F) > Fmax) sat += T;
            F = std::clamp(F, -Fmax, Fmax);
            for (int i = 0; i < 10; ++i) s = rk4(p, s, F, T / 10);
            fell = std::fabs(s[2]) > 1.0;
        }
        std::printf("  %4.1fx  %.1f+/-%.1fj, %5.1f, %5.1f  %12.1f  %16.2f  %s\n",
                    alpha, -2 * alpha, 1 * alpha, -5 * alpha, -6 * alpha, peak, sat,
                    fell ? "FELL" : "balanced");
    }
    return 0;
}

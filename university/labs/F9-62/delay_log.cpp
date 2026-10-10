// delay_log.cpp - evidence generator for the RB402 course forensic lab ("stable in simulation,
// unstable on the robot"). The robot applies each command several samples after it was
// computed; the team's simulator does not. Both use the same LQR gain and start.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <deque>
#include <string>
#include "cartpole.hpp"
#include "lin.hpp"
#include "lqr.hpp"

int main()
{
    const CartPole p;
    const double T = 0.01, Fmax = 10.0;
    const int lag = 4;  // fault injected: samples between computing and applying a command
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    const Mat K = dlqr(Ad, Bd, diag({25.0, 1.0, 100.0, 1.0}), diag({0.01})).K;
    auto control = [&](const State& s) {
        double F = 0.0;
        for (std::size_t i = 0; i < 4; ++i) F -= K(0, i) * s[i];
        return std::clamp(F, -Fmax, Fmax);
    };
    State sim{0.0, 0.0, 0.02, 0.0};
    State bot = sim;
    std::deque<double> queue(lag, 0.0);  // commands in flight; the robot applies the oldest
    double wSim = 0.0, wBot = 0.0, lastF = 0.0;
    int atLimit = 0, flips = 0;
    std::printf("Part 1: every 0.02 s. Same gain K, same start (th = 0.02 rad).\n");
    std::printf("seq = command sequence number; the driver reports which seq it is executing\n");
    std::printf("  t(s)   th_sim  |  th_robot  x_robot  F_cmd(N)  seq_sent  seq_applied"
                "  F_applied(N)\n");
    for (int k = 0; k <= 300; ++k) {
        const double Fsim = control(sim);
        const double Fcmd = control(bot);
        queue.push_back(Fcmd);
        const double Fapp = queue.front();
        queue.pop_front();
        const int applied = k - lag;
        if (k <= 100 && k % 2 == 0)
            std::printf(" %5.2f  %7.4f  | %9.4f  %7.3f  %8.2f  %8d  %11s  %12.2f\n",
                        k * T, sim[2], bot[2], bot[0], Fcmd, k,
                        applied >= 0 ? std::to_string(applied).c_str() : "(none)", Fapp);
        if (k == 100) {
            std::printf("\nPart 2: summary per 0.5 s window\n");
            std::printf("  window(s)   max|th_sim|  max|th_robot|  samples at 10 N"
                        "  F_applied sign flips\n");
        }
        if (k > 100) {
            wSim = std::fmax(wSim, std::fabs(sim[2]));
            wBot = std::fmax(wBot, std::fabs(bot[2]));
            if (std::fabs(Fapp) >= Fmax) ++atLimit;
            if (Fapp * lastF < 0.0) ++flips;
            if ((k - 100) % 50 == 0) {
                std::printf("  %4.1f-%3.1f   %11.4f  %13.4f  %12d/50  %20d\n",
                            (k - 50) * T, k * T, wSim, wBot, atLimit, flips);
                wSim = wBot = 0.0;
                atLimit = flips = 0;
            }
        }
        lastF = Fapp;
        for (int i = 0; i < 10; ++i) {
            sim = rk4(p, sim, Fsim, T / 10);
            bot = rk4(p, bot, Fapp, T / 10);
        }
    }
    return 0;
}

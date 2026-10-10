// F9-20 Listing 2: a manual tuning session, one change at a time, on the test bench
// of Listing 1. Each row is one experiment; the last column says whether the
// written specification is met.
#include "cart_sim.hpp"
#include <format>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    struct Trial
    {
        std::string why;
        Gains g;
    };
    const std::vector<Trial> trials = {
        {"1 P only", {8.0, 0.0, 0.0}},
        {"2 add D for damping", {8.0, 0.0, 4.8}},
        {"3 add I for the slope", {8.0, 4.0, 4.8}},
        {"4 halve I", {8.0, 2.0, 4.8}},
        {"5 more D", {8.0, 2.0, 6.0}},
        {"6 a little less I", {8.0, 1.5, 6.0}},
        {"7 between 4 and 6", {8.0, 1.75, 5.5}},
    };
    std::cout << "experiment               Kp   Ki     Kd  | rise(s) overshoot settle(s) final err "
                 " peak asked | spec\n";
    for (const Trial& tr : trials) {
        const Metrics m = simulate(tr.g);
        std::cout << std::format("{:<22} {:>4.1f} {:>4.2f} {:>5.1f} | {:>7.2f} {:>7.1f} % {:>9.2f} "
                                 "{:>7.1f} mm {:>8.2f} N | {}\n",
                                 tr.why, tr.g.kp, tr.g.ki, tr.g.kd, m.riseTime, m.overshoot,
                                 m.settlingTime, m.finalError * 1000.0, m.peakAsked,
                                 meetsSpec(m) ? "PASS" : "fail");
    }
    return 0;
}

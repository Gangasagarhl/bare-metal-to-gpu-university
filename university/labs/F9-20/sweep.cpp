// F9-20 Listing 3: a gain sweep. Try every combination on a grid, keep those that meet
// the specification, and rank them by the integral of absolute error (IAE).
#include "cart_sim.hpp"
#include <algorithm>
#include <format>
#include <iostream>
#include <vector>

int main()
{
    struct Candidate
    {
        Gains g;
        Metrics m;
    };
    std::vector<Candidate> passing;
    int tried = 0;
    for (double kp = 5.0; kp <= 10.0 + 1e-9; kp += 1.0) {
        for (double ki = 0.0; ki <= 4.0 + 1e-9; ki += 0.25) {
            for (double kd = 0.0; kd <= 12.0 + 1e-9; kd += 0.5) {
                const Gains g{kp, ki, kd};
                const Metrics m = simulate(g);
                ++tried;
                if (meetsSpec(m)) {
                    passing.push_back({g, m});
                }
            }
        }
    }
    std::sort(passing.begin(), passing.end(),
              [](const Candidate& a, const Candidate& b) { return a.m.iae < b.m.iae; });
    std::cout << std::format("tried {} gain sets, {} meet the specification\n", tried,
                             passing.size());
    std::cout << "rank   Kp   Ki     Kd  | IAE (m s) overshoot settle(s) final err  peak asked\n";
    for (std::size_t i = 0; i < passing.size() && i < 8; ++i) {
        const Candidate& c = passing[i];
        std::cout << std::format("{:>4} {:>4.1f} {:>4.2f} {:>5.1f} | {:>9.3f} {:>7.1f} % {:>9.2f} "
                                 "{:>7.1f} mm {:>8.2f} N\n",
                                 i + 1, c.g.kp, c.g.ki, c.g.kd, c.m.iae, c.m.overshoot,
                                 c.m.settlingTime, c.m.finalError * 1000.0, c.m.peakAsked);
    }
    return 0;
}

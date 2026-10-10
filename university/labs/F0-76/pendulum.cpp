// F0-76 Listing 1 (course lab): integrate a pendulum with two methods (plus a third), h = 0.01 s.
#include <cmath>
#include <cstdio>

#include "pendulum.hpp"

int main()
{
    const double h = 0.01;
    const State start{1.0, 0.0}; // released from 1 rad (about 57 degrees), at rest
    State eu = start, si = start, rk = start;
    State ref = start; // reference: RK4 with a 100 times smaller step
    const double e0 = energy(start);
    std::printf("h = %.3g s, start theta = %.3g rad, energy E0 = %.6f\n", h, start.theta, e0);
    std::printf("%-5s %-10s %-10s %-10s %-10s | %-9s %-9s %-9s\n", "t s", "theta ref", "Euler",
                "semi-impl", "RK4", "E/E0 Eul", "E/E0 semi", "E/E0 RK4");
    for (int step = 1; step <= 1000; ++step) { // 10 s
        eu = eulerStep(eu, h);
        si = semiImplicitStep(si, h);
        rk = rk4Step(rk, h);
        for (int k = 0; k < 100; ++k) {
            ref = rk4Step(ref, h / 100.0);
        }
        if (step % 100 == 0) {
            std::printf("%-5.1f %-10.5f %-10.5f %-10.5f %-10.5f | %-9.5f %-9.5f %-11.9f\n",
                        step * h, ref.theta, eu.theta, si.theta, rk.theta, energy(eu) / e0,
                        energy(si) / e0, energy(rk) / e0);
        }
    }
    return 0;
}

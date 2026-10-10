// F0-76 Listing 2: measure the order of accuracy. Halve h and watch how the error at t = 2 s
// shrinks.
#include <cmath>
#include <cstdio>

#include "pendulum.hpp"

template <typename Step> State run(Step step, double h, int n)
{
    State s{1.0, 0.0};
    for (int i = 0; i < n; ++i) {
        s = step(s, h);
    }
    return s;
}

int main()
{
    const double T = 2.0;
    const State ref = run(rk4Step, T / 200000.0, 200000); // h = 1e-5: reference
    std::printf("reference theta(2 s) = %.12f\n", ref.theta);
    std::printf("%-9s %-12s %-8s %-12s %-8s %-12s %-8s\n", "h", "err Euler", "ratio", "err semi",
                "ratio", "err RK4", "ratio");
    double pe = 0.0, ps = 0.0, pr = 0.0;
    for (int n = 20; n <= 640; n *= 2) {
        const double h = T / n;
        const double ee = std::fabs(run(eulerStep, h, n).theta - ref.theta);
        const double es = std::fabs(run(semiImplicitStep, h, n).theta - ref.theta);
        const double er = std::fabs(run(rk4Step, h, n).theta - ref.theta);
        if (n == 20) {
            std::printf("%-9.5f %-12.3e %-8s %-12.3e %-8s %-12.3e %-8s\n", h, ee, "-", es, "-", er,
                        "-");
        } else {
            std::printf("%-9.5f %-12.3e %-8.2f %-12.3e %-8.2f %-12.3e %-8.2f\n", h, ee, pe / ee, es,
                        ps / es, er, pr / er);
        }
        pe = ee;
        ps = es;
        pr = er;
    }
    return 0;
}

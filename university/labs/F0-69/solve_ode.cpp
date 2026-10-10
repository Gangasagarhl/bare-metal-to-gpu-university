// F0-69 Listing 2: two differential equations solved with the Laplace transform in the
// chapter, checked against a step-by-step simulation.
//   (a) y' + 2 y = 4, y(0) = 1          ->  Y = 2/s - 1/(s+2)              ->  y = 2 - e^(-2t)
//   (b) y'' + 3 y' + 2 y = 2, y(0) = y'(0) = 0
//                                       ->  Y = 1/s - 2/(s+1) + 1/(s+2)   ->  y = 1 - 2e^(-t) + e^(-2t)
#include <cmath>
#include <format>
#include <iostream>

int main()
{
    const double dt = 1e-5;
    double ya = 1.0;
    double yb = 0.0;
    double vb = 0.0;
    double worstA = 0.0;
    double worstB = 0.0;
    std::cout << "  t (s)   (a) sim   (a) Laplace   (b) sim   (b) Laplace\n";
    const int n = static_cast<int>(std::lround(5.0 / dt));
    for (int k = 0; k <= n; ++k) {
        const double t = k * dt;
        const double exactA = 2.0 - std::exp(-2.0 * t);
        const double exactB = 1.0 - 2.0 * std::exp(-t) + std::exp(-2.0 * t);
        worstA = std::max(worstA, std::abs(ya - exactA));
        worstB = std::max(worstB, std::abs(yb - exactB));
        if (k % 50000 == 0) {
            std::cout << std::format("{:>7.1f} {:>9.5f} {:>13.5f} {:>9.5f} {:>13.5f}\n", t, ya, exactA, yb, exactB);
        }
        ya += (4.0 - 2.0 * ya) * dt;
        const double ab = 2.0 - 3.0 * vb - 2.0 * yb;
        yb += vb * dt;
        vb += ab * dt;
    }
    std::cout << std::format("largest differences: (a) {:.2e}, (b) {:.2e}\n", worstA, worstB);
    std::cout << "final values from the final value theorem: (a) lim s Y(s) = 2, (b) lim s Y(s) = 1\n";
    const bool pass = worstA < 1e-4 && worstB < 1e-4;
    std::cout << (pass ? "PASS" : "FAIL") << ": Laplace solutions match the simulation\n";
    return pass ? 0 : 1;
}

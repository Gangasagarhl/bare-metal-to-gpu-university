// F0-79 Listing 2: where the pole of w' = -w / tau lands after each discretisation rule.
// Exact (zero-order hold) z = e^(-T/tau). A discrete system is stable when |z| < 1.
#include <cmath>
#include <cstdio>

int main()
{
    std::printf("%-7s %-10s %-11s %-10s %-10s\n", "T/tau", "exact", "fwd Euler", "bwd Euler",
                "Tustin");
    const double ratios[] = {0.01, 0.1, 0.5, 1.0, 1.5, 2.0, 2.5, 4.0};
    for (double a : ratios) {
        std::printf("%-7.2f %-10.5f %-11.5f %-10.5f %-10.5f\n", a, std::exp(-a), 1.0 - a,
                    1.0 / (1.0 + a), (1.0 - a / 2.0) / (1.0 + a / 2.0));
    }
    return 0;
}

// F0-74 Listing 2: cancellation. Subtracting nearly equal numbers exposes the rounding they
// carried.
#include <cmath>
#include <cstdio>

int main()
{
    // (1 - cos x) / x^2 -> 1/2 as x -> 0. Two formulas that are equal in exact arithmetic.
    std::printf("%-8s %-24s %-24s\n", "x", "(1 - cos x)/x^2", "2 sin^2(x/2)/x^2");
    for (double x = 1.0e-1; x >= 1.0e-9; x /= 10.0) {
        const double naive = (1.0 - std::cos(x)) / (x * x);
        const double s = std::sin(x / 2.0);
        const double stable = 2.0 * s * s / (x * x);
        std::printf("%-8.0e %-24.17g %-24.17g\n", x, naive, stable);
    }

    // A forward difference (sin(1 + h) - sin(1)) / h approximates cos(1).
    // Small h: less truncation error, more cancellation. The total error is U-shaped.
    std::printf("\n%-8s %-24s %-12s\n", "h", "forward difference", "abs. error");
    const double x = 1.0, truth = std::cos(1.0);
    for (int k = 1; k <= 15; ++k) {
        const double h = std::pow(10.0, -k);
        const double d = (std::sin(x + h) - std::sin(x)) / h;
        std::printf("%-8.0e %-24.17g %-12.3g\n", h, d, std::fabs(d - truth));
    }
    return 0;
}

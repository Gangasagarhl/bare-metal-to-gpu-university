// F0-74 Listing 1: each operation rounds; errors add up as a loop repeats the operation.
#include <cmath>
#include <cstdio>

int main()
{
    const double u = std::ldexp(1.0, -24); // unit roundoff of float, round to nearest: 2^-24
    std::printf("unit roundoff u(float) = %.6g\n\n", u);

    // One operation: fl(a + b) = (a + b)(1 + d) with |d| <= u.
    const float a = 0.1f, b = 0.2f;
    const double exact = static_cast<double>(a) + static_cast<double>(b); // exact: fits in double
    const float rounded = a + b;
    std::printf(
        "a + b exact   = %.17g\nfl(a + b)     = %.17g\nrelative error d = %.3g  (|d| <= u: %s)\n\n",
        exact, static_cast<double>(rounded), (static_cast<double>(rounded) - exact) / exact,
        std::fabs((static_cast<double>(rounded) - exact) / exact) <= u ? "yes" : "no");

    // Many operations: add the float 0.1f to a running float total n times.
    std::printf("%-10s %-16s %-20s %-12s %-12s\n", "n", "float total", "exact n * 0.1f",
                "rel. error", "bound n*u");
    const double tenth = static_cast<double>(0.1f); // the value really being added
    long n = 1;
    float total = 0.0f;
    for (long i = 1; i <= 10000000; ++i) {
        total += 0.1f;
        if (i == n) {
            const double want =
                static_cast<double>(i) * tenth; // exact enough: error < 1e-16 relative
            const double rel = std::fabs(static_cast<double>(total) - want) / want;
            std::printf("%-10ld %-16.9g %-20.12g %-12.3g %-12.3g\n", i, static_cast<double>(total),
                        want, rel, static_cast<double>(i) * u);
            n *= 10;
        }
    }
    return 0;
}

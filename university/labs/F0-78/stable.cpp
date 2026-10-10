// F0-78 Listing 2: well-conditioned problems, unstable algorithms (and their stable replacements).
#include <cmath>
#include <cstdio>

// e^x by summing its Taylor series term by term until the terms stop changing the sum.
double expSeries(double x)
{
    double sum = 1.0, term = 1.0;
    for (int k = 1; k < 200; ++k) {
        term *= x / k;
        sum += term;
    }
    return sum;
}

int main()
{
    // Problem 1: the small root of x^2 - b x + 1 = 0 for large b (well-conditioned: about 1/b).
    std::printf("%-8s %-24s %-24s\n", "b", "textbook (b - sqrt(d))/2", "stable 2/(b + sqrt(d))");
    for (double b = 1.0e2; b <= 1.0e10; b *= 100.0) {
        const double d = b * b - 4.0;
        const double textbook = (b - std::sqrt(d)) / 2.0;
        const double stable = 2.0 / (b + std::sqrt(d));
        std::printf("%-8.0e %-24.17g %-24.17g\n", b, textbook, stable);
    }
    // Problem 2: e^-20 (well-conditioned: relative condition number |x| = 20).
    std::printf("\nexp(-20) library       = %.17g\n", std::exp(-20.0));
    std::printf("series at x = -20      = %.17g\n", expSeries(-20.0));
    std::printf("1 / (series at x = 20) = %.17g\n", 1.0 / expSeries(20.0));
    std::printf("largest series term at x = -20: 20^20/20! = %.6g\n",
                std::pow(20.0, 20) / std::tgamma(21.0));
    return 0;
}

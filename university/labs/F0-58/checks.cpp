// F0-58 checks: recompute every number used in the chapter text.
#include <cmath>
#include <iostream>
#include <vector>

void describe(const char* label, const std::vector<double>& x)
{
    double sum = 0.0;
    for (double v : x) {
        sum += v;
    }
    const double n = static_cast<double>(x.size());
    const double mean = sum / n;
    double ss = 0.0;
    for (double v : x) {
        ss += (v - mean) * (v - mean);
    }
    std::cout << label << ": mean " << mean << ", variance (n) " << ss / n << ", variance (n-1) "
              << ss / (n - 1.0) << ", std dev (n-1) " << std::sqrt(ss / (n - 1.0)) << "\n";
}

int main()
{
    // One fair die: E[X], E[X^2], Var(X).
    double e = 0.0;
    double e2 = 0.0;
    for (int k = 1; k <= 6; ++k) {
        e += k / 6.0;
        e2 += k * k / 6.0;
    }
    std::cout << "die: E[X] = " << e << ", E[X^2] = " << e2 << ", Var = " << e2 - e * e
              << ", sigma = " << std::sqrt(e2 - e * e) << "\n";
    std::cout << "two dice total: mean " << 2 * e << ", variance " << 2 * (e2 - e * e)
              << ", sigma " << std::sqrt(2 * (e2 - e * e)) << "\n";
    // Celsius to Fahrenheit: F = 1.8 C + 32.
    std::cout << "20 C with sd 0.5 C -> mean " << 1.8 * 20 + 32 << " F, sd " << 1.8 * 0.5 << " F\n";
    // Averaging: sd 0.4 per reading.
    std::cout << "sd of mean of 25 readings with sd 0.4: " << 0.4 / std::sqrt(25.0) << "\n";
    std::cout << "readings needed for sd 0.1 from sd 0.4: " << std::pow(0.4 / 0.1, 2) << "\n";
    // Chebyshev bound for k = 2 and k = 3.
    std::cout << "Chebyshev: k = 2 -> at most " << 1.0 / 4
              << ", k = 3 -> at most " << 1.0 / 9 << "\n";
    // Check-yourself data.
    describe("4 8 6 5 7", {4, 8, 6, 5, 7});
    describe("kettle + outlier 260",
             {182, 176, 190, 185, 179, 188, 181, 184, 177, 186, 260});
    std::cout << "Bernoulli(0.2): mean 0.2, variance " << 0.2 * 0.8 << "\n";
    std::cout << "Binomial(10, 0.5): mean " << 10 * 0.5 << ", variance " << 10 * 0.5 * 0.5
              << ", sd " << std::sqrt(2.5) << "\n";
    return 0;
}

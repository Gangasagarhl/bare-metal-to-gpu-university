// F9-36 checks: the worked example's arithmetic and the cost figures quoted in the text.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    const std::vector<double> lik = {0.9, 0.9, 0.05, 0.9, 0.05, 0.05};
    double total = 0.0;
    for (double l : lik) {
        total += l / 6.0;
    }
    std::cout << "sum of unnormalised weights = " << total << '\n';
    double cumulative = 0.0, sumSq = 0.0;
    for (double l : lik) {
        const double w = l / 6.0 / total;
        cumulative += w;
        sumSq += w * w;
        std::cout << "w = " << w << "  cumulative = " << cumulative << '\n';
    }
    std::cout << "ESS = " << 1.0 / sumSq << '\n';
    std::cout << "ESS with equal weights (N = 500) = "
              << 1.0 / (500.0 * (1.0 / 500.0) * (1.0 / 500.0))
              << "; ESS with one particle holding all weight = 1\n";
    // Work per step: grid filter of Listing 2 versus particle filter.
    std::cout << "grid prediction: 300 cells x 26 kernel entries = " << 300 * 26
              << " multiply-adds per step; particle filter: 500 samples, 500 likelihoods\n";
    // Expected number of distinct particles after resampling from uniform weights:
    // N (1 - (1 - 1/N)^N), close to N (1 - 1/e).
    const double n = 500.0;
    std::cout << "expected distinct after multinomial resampling of equal weights: "
              << n * (1.0 - std::pow(1.0 - 1.0 / n, n)) << " of 500\n";
    return 0;
}

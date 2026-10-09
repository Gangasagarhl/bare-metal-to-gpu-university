// F0-57 Listing 1: X = number of heads in n fair coin tosses. Exact PMF and CDF by
// enumeration and by formula, then a simulated histogram of 10000 experiments (n = 10).
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

double choose(int n, int k)
{
    double result = 1.0;
    for (int i = 1; i <= k; ++i) {
        result = result * (n - k + i) / i;
    }
    return result;
}

double binomialPmf(int n, int k, double p)
{
    return choose(n, k) * std::pow(p, k) * std::pow(1.0 - p, n - k);
}

int main()
{
    // Part 1: n = 4 by enumeration. Each outcome is a 4-bit pattern; bit = 1 means heads.
    const int n4 = 4;
    std::vector<int> count(n4 + 1, 0);
    for (int pattern = 0; pattern < (1 << n4); ++pattern) {
        int heads = 0;
        for (int bit = 0; bit < n4; ++bit) {
            heads += (pattern >> bit) & 1;
        }
        ++count[heads];
    }
    std::cout << "n = 4 tosses, 16 equally likely outcomes\n";
    std::cout << "k  outcomes  P(X=k)  formula   P(X<=k)\n";
    double cdf = 0.0;
    for (int k = 0; k <= n4; ++k) {
        const double p = count[k] / 16.0;
        cdf += p;
        std::cout << k << "  " << count[k] << "         " << p << "  "
                  << binomialPmf(n4, k, 0.5) << "    " << cdf << "\n";
    }

    // Part 2: n = 10, exact PMF next to the frequencies of 10000 simulated experiments.
    const int n = 10;
    const int experiments = 10000;
    std::mt19937 engine(57);
    std::vector<int> hist(n + 1, 0);
    for (int e = 0; e < experiments; ++e) {
        int heads = 0;
        for (int t = 0; t < n; ++t) {
            heads += static_cast<int>(engine() >> 31);  // top bit: 0 or 1, each half the time
        }
        ++hist[heads];
    }
    std::cout << "\nn = 10 tosses, " << experiments << " simulated experiments (seed 57)\n";
    std::cout << "k   exact P(X=k)  frequency  bar (one # per 0.01)\n";
    for (int k = 0; k <= n; ++k) {
        const double freq = static_cast<double>(hist[k]) / experiments;
        std::cout << (k < 10 ? " " : "") << k << "  " << binomialPmf(n, k, 0.5) << "  "
                  << freq << "  " << std::string(static_cast<std::size_t>(freq * 100 + 0.5), '#')
                  << "\n";
    }
    return 0;
}

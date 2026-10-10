// odds.cpp - F12-17 Listing 3: predicting bisect.cpp's success rate before running it.
// On the path to build 41 the bisection tests builds 31, 47, 39, 43, 41, 40: three good
// and three bad. It returns 41 only if all six verdicts are right. A good build (20 ms) is
// misjudged if its median is above 21.5 ms; a bad one (23 ms) if its median is <= 21.5 ms.
#include <cmath>
#include <cstdio>
#include <initializer_list>

// Probability that one noisy measurement gives the wrong verdict. The noise in bisect.cpp
// takes the 2001 values (u - 1000) / 10000 for u = 0 .. 2000, all equally likely.
double single_wrong(double truth)
{
    int wrong = 0;
    for (int u = 0; u <= 2000; ++u) {
        const double m = truth * (1.0 + (u - 1000) / 10000.0);
        const bool says_bad = m > 21.5;
        wrong += says_bad != (truth > 21.5);
    }
    return wrong / 2001.0;
}

// The median of k (k odd) is wrong when more than half of the k measurements are wrong.
double median_wrong(double p, int k)
{
    double sum = 0.0;
    for (int j = k / 2 + 1; j <= k; ++j) {
        sum += std::tgamma(k + 1.0) / (std::tgamma(j + 1.0) * std::tgamma(k - j + 1.0)) *
               std::pow(p, j) * std::pow(1.0 - p, k - j);
    }
    return sum;
}

int main()
{
    const double pg = single_wrong(20.0), pb = single_wrong(23.0);
    std::printf("one measurement wrong: good build %.4f, bad build %.4f\n", pg, pb);
    for (const int k : {1, 3, 5, 7, 9}) {
        const double qg = median_wrong(pg, k), qb = median_wrong(pb, k);
        const double right = std::pow(1.0 - qg, 3) * std::pow(1.0 - qb, 3);
        std::printf("k = %d: step wrong %.4f (good) %.4f (bad); predicted right in %.1f of 1000\n",
                    k, qg, qb, 1000.0 * right);
    }
    return 0;
}

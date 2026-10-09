// stats.cpp - summary statistics for a list of run times (one number per line on stdin).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <vector>

// Nearest-rank percentile: the smallest value with at least p % of the data at or below it.
double percentile(std::vector<double> const& sorted, double p)
{
    auto const n = static_cast<double>(sorted.size());
    auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * n));
    rank = std::clamp<std::size_t>(rank, 1, sorted.size());
    return sorted[rank - 1];
}

int main()
{
    std::vector<double> t;
    double x = 0.0;
    while (std::cin >> x) {
        t.push_back(x);
    }
    if (t.empty()) {
        std::puts("no data");
        return 1;
    }
    std::vector<double> s = t;
    std::sort(s.begin(), s.end());
    double sum = 0.0;
    for (double v : t) {
        sum += v;
    }
    double const mean = sum / static_cast<double>(t.size());
    double sq = 0.0;
    for (double v : t) {
        sq += (v - mean) * (v - mean);
    }
    double const sd = t.size() > 1 ? std::sqrt(sq / static_cast<double>(t.size() - 1)) : 0.0;
    std::size_t const n = s.size();
    double const median = (n % 2 == 1) ? s[n / 2] : (s[n / 2 - 1] + s[n / 2]) / 2.0;
    std::printf("n       %zu\n", n);
    std::printf("min     %.2f\n", s.front());
    std::printf("median  %.2f\n", median);
    std::printf("p90     %.2f\n", percentile(s, 90.0));
    std::printf("max     %.2f\n", s.back());
    std::printf("mean    %.2f\n", mean);
    std::printf("stddev  %.2f\n", sd);
    return 0;
}

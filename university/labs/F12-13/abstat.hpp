// abstat.hpp - F12-13: compare two benchmark variants honestly.
// median, and a percentile-bootstrap confidence interval for the ratio median(B) / median(A).
// The bootstrap resamples each variant's runs with replacement many times, recomputes the
// ratio each time, and reports the middle 95 % of those ratios. Fixed seed: same data, same
// interval, on every machine.
#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

namespace abstat
{

struct Rng
{
    std::uint64_t s;
    std::uint64_t next()
    {
        std::uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    std::size_t below(std::size_t n) { return static_cast<std::size_t>(next() % n); }
};

inline double median(std::vector<double> v)
{
    std::sort(v.begin(), v.end());
    std::size_t const n = v.size();
    return n % 2 == 1 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

struct Interval
{
    double ratio;  // median(b) / median(a) of the data itself
    double low;    // 2.5 % point of the bootstrap ratios
    double high;   // 97.5 % point of the bootstrap ratios
};

inline Interval ratio_ci(std::vector<double> const& a, std::vector<double> const& b,
                         int resamples = 10000, std::uint64_t seed = 13)
{
    Rng rng{seed};
    std::vector<double> ratios;
    ratios.reserve(static_cast<std::size_t>(resamples));
    std::vector<double> ra(a.size());
    std::vector<double> rb(b.size());
    for (int r = 0; r < resamples; ++r) {
        for (double& x : ra) {
            x = a[rng.below(a.size())];
        }
        for (double& x : rb) {
            x = b[rng.below(b.size())];
        }
        ratios.push_back(median(rb) / median(ra));
    }
    std::sort(ratios.begin(), ratios.end());
    auto at = [&](double q) {
        return ratios[static_cast<std::size_t>(q * static_cast<double>(ratios.size() - 1))];
    };
    return {median(b) / median(a), at(0.025), at(0.975)};
}

}  // namespace abstat

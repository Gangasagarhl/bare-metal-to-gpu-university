// benchstats.hpp - MA202 course project: the benchmark-statistics helper (chapter F0-63).
// Summarises repeated timings with order statistics: median, nearest-rank percentiles,
// interquartile range (IQR) and Tukey fences, plus mean and sd for comparison.
// Definitions match SP302 F2-51: median = middle value (mean of the two middle values
// for even n); p-th percentile = value at rank ceil(p/100 * n) of the sorted data.
#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace benchstats
{

struct Summary
{
    std::size_t n;
    double minimum;
    double q1;            // 25th percentile (nearest rank)
    double median;
    double q3;            // 75th percentile (nearest rank)
    double p90;           // 90th percentile (nearest rank)
    double maximum;
    double mean;
    double sd;            // sample standard deviation (divides by n - 1)
    double iqr;           // q3 - q1
    double lowFence;      // q1 - 1.5 * iqr
    double highFence;     // q3 + 1.5 * iqr
    std::size_t outliersLow;
    std::size_t outliersHigh;
};

// Nearest-rank percentile of SORTED data, 0 < p <= 100.
inline double percentile(std::vector<double> const& sorted, double p)
{
    if (sorted.empty()) {
        throw std::invalid_argument("percentile of no data");
    }
    auto const n = static_cast<double>(sorted.size());
    auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * n));
    rank = std::clamp<std::size_t>(rank, 1, sorted.size());
    return sorted[rank - 1];
}

// Median of SORTED data.
inline double median(std::vector<double> const& sorted)
{
    if (sorted.empty()) {
        throw std::invalid_argument("median of no data");
    }
    std::size_t const n = sorted.size();
    return n % 2 == 1 ? sorted[n / 2] : (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0;
}

inline Summary summarise(std::vector<double> values)
{
    if (values.empty()) {
        throw std::invalid_argument("summary of no data");
    }
    std::sort(values.begin(), values.end());
    Summary s{};
    s.n = values.size();
    s.minimum = values.front();
    s.maximum = values.back();
    s.q1 = percentile(values, 25.0);
    s.median = median(values);
    s.q3 = percentile(values, 75.0);
    s.p90 = percentile(values, 90.0);
    double sum = 0.0;
    for (double v : values) {
        sum += v;
    }
    s.mean = sum / static_cast<double>(s.n);
    double sq = 0.0;
    for (double v : values) {
        sq += (v - s.mean) * (v - s.mean);
    }
    s.sd = s.n > 1 ? std::sqrt(sq / static_cast<double>(s.n - 1)) : 0.0;
    s.iqr = s.q3 - s.q1;
    s.lowFence = s.q1 - 1.5 * s.iqr;
    s.highFence = s.q3 + 1.5 * s.iqr;
    for (double v : values) {
        s.outliersLow += v < s.lowFence ? 1 : 0;
        s.outliersHigh += v > s.highFence ? 1 : 0;
    }
    return s;
}

inline void print(std::ostream& out, Summary const& s, char const* unit)
{
    out << "n " << s.n << " | min " << s.minimum << " | q1 " << s.q1 << " | median " << s.median
        << " | q3 " << s.q3 << " | p90 " << s.p90 << " | max " << s.maximum << " (" << unit
        << ")\n";
    out << "IQR " << s.iqr << " (" << 100.0 * s.iqr / s.median << " % of median)"
        << " | fences [" << s.lowFence << ", " << s.highFence << "] | outliers "
        << s.outliersLow << " low, " << s.outliersHigh << " high | mean " << s.mean
        << " | sd " << s.sd << "\n";
}

// Ratio of medians b/a, and whether the two middle halves [q1, q3] overlap.
// Overlapping middle halves mean: do not claim a difference from these runs.
struct Comparison
{
    double ratio;
    bool overlap;
};

inline Comparison compare(Summary const& a, Summary const& b)
{
    return {b.median / a.median, !(b.q3 < a.q1 || a.q3 < b.q1)};
}

// Tell the optimiser that 'value' is used (GNU extended asm, emits no instructions).
template <typename T>
inline void keep(T const& value)
{
    asm volatile("" : : "r,m"(value) : "memory");
}

// Run work() 'warmups' times untimed, then 'reps' times timed; returns milliseconds,
// in the order measured (keep the raw list: it is the evidence).
template <typename Work>
std::vector<double> timeRuns(Work&& work, int warmups, int reps)
{
    using clock = std::chrono::steady_clock;
    for (int i = 0; i < warmups; ++i) {
        work();
    }
    std::vector<double> ms;
    ms.reserve(static_cast<std::size_t>(reps));
    for (int i = 0; i < reps; ++i) {
        auto const start = clock::now();
        work();
        auto const stop = clock::now();
        ms.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
    }
    return ms;
}

}  // namespace benchstats

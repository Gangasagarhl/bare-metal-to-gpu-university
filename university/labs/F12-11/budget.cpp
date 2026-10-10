// budget.cpp - F12-11 Listing 1: splitting an end-to-end latency requirement into a budget.
// A request passes four stages one after another. Each stage's time is drawn from an
// EXERCISE MODEL (a fixed part plus an exponential part; not a measurement of any system).
// The program shows two facts a latency budget must respect:
//   1. means add up exactly, percentiles do not (the sum of the stages' p99 values is
//      larger than the p99 of the whole request when the stages vary independently);
//   2. a request that waits for the slowest of k parallel calls meets each call's p99
//      far less often than 99 % of the time: 1 - 0.99^k of requests see at least one slow call.
// Percentiles are nearest-rank: the value at rank ceil(p/100 * n) of the sorted samples.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// Deterministic generator (SplitMix64), written out in full so that the samples, and therefore
// every number printed, are the same on every machine and with every standard library.
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
    double uniform() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }
    double exponential(double mean) { return -mean * std::log(1.0 - uniform()); }
};

struct Stage
{
    std::string name;
    double fixed_ms;  // the part every request pays
    double exp_ms;    // mean of the exponential (variable) part
};

double percentile(std::vector<double> v, double p)
{
    std::size_t rank =
        static_cast<std::size_t>(std::ceil(p / 100.0 * static_cast<double>(v.size())));
    rank = std::max<std::size_t>(rank, 1);
    std::nth_element(v.begin(), v.begin() + static_cast<long>(rank - 1), v.end());
    return v[rank - 1];
}

double mean(std::vector<double> const& v)
{
    double sum = 0.0;
    for (double x : v) {
        sum += x;
    }
    return sum / static_cast<double>(v.size());
}

int main()
{
    std::vector<Stage> const stages = {
        {"gateway", 0.3, 0.2}, {"auth", 0.5, 0.5}, {"store", 1.0, 2.0}, {"replicate", 2.0, 1.5}};
    std::size_t const n = 200000;
    Rng rng{11};

    std::vector<std::vector<double>> per_stage(stages.size(), std::vector<double>(n));
    std::vector<double> total(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t s = 0; s < stages.size(); ++s) {
            double const t = stages[s].fixed_ms + rng.exponential(stages[s].exp_ms);
            per_stage[s][i] = t;
            total[i] += t;
        }
    }

    std::printf("== Part 1: four stages in series, %zu simulated requests (exercise model)\n", n);
    std::printf("%-10s %9s %9s %9s\n", "stage", "mean ms", "p50 ms", "p99 ms");
    double sum_mean = 0.0;
    double sum_p99 = 0.0;
    for (std::size_t s = 0; s < stages.size(); ++s) {
        double const m = mean(per_stage[s]);
        double const p99 = percentile(per_stage[s], 99.0);
        sum_mean += m;
        sum_p99 += p99;
        std::printf("%-10s %9.2f %9.2f %9.2f\n", stages[s].name.c_str(), m,
                    percentile(per_stage[s], 50.0), p99);
    }
    std::printf("%-10s %9.2f %9s %9.2f   <- sums of the column above\n", "sum", sum_mean, "",
                sum_p99);
    std::printf("%-10s %9.2f %9.2f %9.2f   <- the whole request, measured end to end\n", "end2end",
                mean(total), percentile(total, 50.0), percentile(total, 99.0));
    std::printf(
        "means add up (%.2f vs %.2f); p99 values do not (%.2f summed vs %.2f end to end)\n\n",
        sum_mean, mean(total), sum_p99, percentile(total, 99.0));

    std::printf("== Part 2: fan-out, the request waits for the slowest of k parallel calls\n");
    std::printf("each call: 1.0 ms fixed + exponential with mean 2.0 ms; its own p99 = %.2f ms\n",
                1.0 + 2.0 * std::log(100.0));
    std::printf("%5s %22s %22s %16s\n", "k", "requests with a call", "predicted 1-0.99^k",
                "request p99 ms");
    std::printf("%5s %22s %22s %16s\n", "", "slower than its p99", "", "");
    double const call_p99 = 1.0 + 2.0 * std::log(100.0);
    for (int k : {1, 5, 10, 50, 100}) {
        std::size_t const m = 50000;
        std::vector<double> slowest(m);
        std::size_t hit = 0;
        for (std::size_t i = 0; i < m; ++i) {
            double worst = 0.0;
            for (int c = 0; c < k; ++c) {
                worst = std::max(worst, 1.0 + rng.exponential(2.0));
            }
            slowest[i] = worst;
            if (worst > call_p99) {
                ++hit;
            }
        }
        std::printf("%5d %21.1f%% %21.1f%% %16.2f\n", k,
                    100.0 * static_cast<double>(hit) / static_cast<double>(m),
                    100.0 * (1.0 - std::pow(0.99, k)), percentile(slowest, 99.0));
    }
    return 0;
}

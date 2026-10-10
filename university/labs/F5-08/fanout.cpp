// fanout.cpp - why a request that asks many servers is slow more often than one server is.
// Model (an exercise, not a measurement): each server answers in 10 ms, except that with
// probability p = 0.01 it answers in 200 ms (a hiccup: a pause, a busy disk, a retransmission).
// A request that asks k servers in parallel must wait for the slowest of them.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

struct Rng
{
    std::uint64_t state;
    std::uint64_t next()
    {
        std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    double uniform() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }
};

double percentile(std::vector<double> v, double pct)   // nearest rank: ceil(pct/100 * n)
{
    std::sort(v.begin(), v.end());
    std::size_t rank = static_cast<std::size_t>(std::ceil(pct / 100.0 * v.size()));
    rank = std::clamp<std::size_t>(rank, 1, v.size());
    return v[rank - 1];
}

int main()
{
    const double p = 0.01;
    const double fastMs = 10.0;
    const double slowMs = 200.0;
    const int requests = 100000;
    Rng rng{7};
    std::printf("server: %.0f ms, or %.0f ms with probability %.2f; %d requests per row\n\n",
                fastMs, slowMs, p, requests);
    std::printf("%5s  %16s  %16s  %10s  %10s\n", "k", "slow (formula)", "slow (simulated)",
                "median ms", "p99 ms");
    for (int k : {1, 2, 10, 50, 100, 500}) {
        std::vector<double> latency;
        latency.reserve(requests);
        int slow = 0;
        for (int r = 0; r < requests; ++r) {
            double worst = 0.0;
            for (int s = 0; s < k; ++s) {
                worst = std::max(worst, rng.uniform() < p ? slowMs : fastMs);
            }
            slow += worst > fastMs ? 1 : 0;
            latency.push_back(worst);
        }
        double none = 1.0;                       // (1 - p)^k: no server hiccups
        for (int s = 0; s < k; ++s) {
            none *= 1.0 - p;
        }
        std::printf("%5d  %16.4f  %16.4f  %10.0f  %10.0f\n", k, 1.0 - none,
                    1.0 * slow / requests, percentile(latency, 50.0), percentile(latency, 99.0));
    }
    return 0;
}

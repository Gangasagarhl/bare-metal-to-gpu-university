// node_model.hpp - F12-15: the exercise model of ONE service node, shared by capacity.cpp and
// outage.cpp. A node has 4 worker threads; each request needs an exponentially distributed
// service time with mean 4 ms; requests arrive as a Poisson stream and wait in one queue,
// first come first served. This is a simulation model chosen for the exercises, not a
// measurement of any real machine. Percentiles are nearest-rank.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <queue>
#include <vector>

namespace node
{

int const kWorkers = 4;
double const kServiceMs = 4.0;

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

inline double percentile(std::vector<double> v, double p)
{
    auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * static_cast<double>(v.size())));
    rank = std::max<std::size_t>(rank, 1);
    std::nth_element(v.begin(), v.begin() + static_cast<long>(rank - 1), v.end());
    return v[rank - 1];
}

// Latencies (ms) of all requests that arrive during 'seconds' at 'rate' req/s on one node.
inline std::vector<double> simulate(double rate, double seconds, std::uint64_t seed)
{
    Rng rng{seed};
    std::priority_queue<double, std::vector<double>, std::greater<>> free_at;
    for (int w = 0; w < kWorkers; ++w) {
        free_at.push(0.0);
    }
    std::vector<double> lat;
    for (double t = rng.exponential(1000.0 / rate); t < seconds * 1000.0;
         t += rng.exponential(1000.0 / rate)) {
        double const start = std::max(t, free_at.top());
        free_at.pop();
        double const done = start + rng.exponential(kServiceMs);
        free_at.push(done);
        lat.push_back(done - t);
    }
    return lat;
}

}  // namespace node

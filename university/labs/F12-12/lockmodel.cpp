// lockmodel.cpp - F12-12 forensic evidence generator: "the model said 40 %".
// A service with 4 worker threads. Each request needs S ms of CPU (exponential, mean 4 ms).
// The first half of that work happens while holding ONE shared index lock; the second half
// (formatting the reply) runs without it. Requests wait for a free worker in arrival order;
// a worker that waits for the lock stays occupied (it is blocked, not free).
// The team's capacity model counted only CPU. This exercise simulation (fixed seed, not a
// measurement) produces the load-test table and thread-state profile they were given.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <queue>
#include <vector>

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

double pct(std::vector<double> v, double p)
{
    auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * static_cast<double>(v.size())));
    std::nth_element(v.begin(), v.begin() + static_cast<long>(rank - 1), v.end());
    return v[rank - 1];
}

int main()
{
    int const workers = 4;
    double const mean_cpu_ms = 4.0;
    double const locked_fraction = 0.5;
    double const seconds = 120.0;

    std::printf("== evidence 1: capacity model in the design review (2026-05-11)\n");
    std::printf("CPU per request (profiled at low load): %.1f ms; worker threads: %d\n",
                mean_cpu_ms, workers);
    std::printf(
        "capacity = %d / %.1f ms = %.0f req/s; target load 400 req/s -> utilisation 40 %%\n",
        workers, mean_cpu_ms, 1000.0 * workers / mean_cpu_ms);
    std::printf("conclusion in the review: \"plenty of headroom; no queueing expected\"\n\n");

    std::printf(
        "== evidence 2: load test, 120 s per row, open-loop Poisson arrivals, gateway timing\n");
    std::printf("%6s %8s %8s %9s %8s | thread states of the 4 workers (%% of time)\n", "req/s",
                "p50 ms", "p99 ms", "max ms", "CPU %");
    std::printf("%6s %8s %8s %9s %8s | %8s %16s %6s\n", "", "", "", "", "", "running",
                "blocked (mutex)", "idle");
    for (double rate : {100.0, 200.0, 300.0, 400.0, 450.0, 480.0}) {
        Rng rng{42};
        std::priority_queue<double, std::vector<double>, std::greater<>> free_at;
        for (int w = 0; w < workers; ++w) {
            free_at.push(0.0);
        }
        double arrival = 0.0;
        double lock_free = 0.0;
        double cpu = 0.0;
        double blocked = 0.0;
        std::vector<double> lat;
        while (arrival < seconds * 1000.0) {
            double const s = rng.exponential(mean_cpu_ms);
            double const start = std::max(arrival, free_at.top());
            free_at.pop();
            double const lock_start = std::max(start, lock_free);
            lock_free = lock_start + locked_fraction * s;
            double const done = lock_free + (1.0 - locked_fraction) * s;
            free_at.push(done);
            cpu += s;
            blocked += lock_start - start;
            lat.push_back(done - arrival);
            arrival += rng.exponential(1000.0 / rate);
        }
        double const capacity = workers * seconds * 1000.0;
        std::printf("%6.0f %8.2f %8.2f %9.1f %7.0f%% | %7.0f%% %15.0f%% %5.0f%%\n", rate,
                    pct(lat, 50.0), pct(lat, 99.0), *std::max_element(lat.begin(), lat.end()),
                    100.0 * cpu / capacity, 100.0 * cpu / capacity, 100.0 * blocked / capacity,
                    std::max(0.0, 100.0 * (1.0 - (cpu + blocked) / capacity)));
    }
    std::printf("\n(\"CPU %%\" is the share of the 4 workers' time spent running code;\n");
    std::printf(" \"blocked (mutex)\" is time a worker held a request but waited for a lock.)\n");
    return 0;
}

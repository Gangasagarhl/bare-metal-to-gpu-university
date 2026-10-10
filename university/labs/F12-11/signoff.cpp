// signoff.cpp - F12-11 forensic evidence generator: "signed off, then paged".
// Produces (1) the acceptance-test report a team ran before release and (2) the production
// dashboard of the first busy day, for the SAME simulated service as reqcheck.cpp:
// one server, first come first served, exponential service time with mean 2 ms.
// Everything is an exercise simulation with fixed seeds, not a measurement of a real system.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
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
    rank = std::max<std::size_t>(rank, 1);
    std::nth_element(v.begin(), v.begin() + static_cast<long>(rank - 1), v.end());
    return v[rank - 1];
}

double avg(std::vector<double> const& v)
{
    double sum = 0.0;
    for (double x : v) {
        sum += x;
    }
    return sum / static_cast<double>(v.size());
}

int main()
{
    Rng rng{7};
    // (1) The acceptance test as the team ran it: one client in a loop, 100 req/s pace, 60 s.
    std::vector<double> test;
    for (double now = 0.0; now < 60000.0;) {
        double const service = rng.exponential(2.0);
        test.push_back(service);
        now += std::max(10.0, service);
    }
    std::printf("== evidence 1: release acceptance test report (2026-03-02)\n");
    std::printf(
        "requirement REQ-7: \"Average response time of the get API shall be under 5 ms.\"\n");
    std::printf(
        "test tool: one client thread, sends a get, waits for the reply, sleeps to keep 100 "
        "req/s\n");
    std::printf("requests: %zu   mean: %.2f ms   p99: %.2f ms   max: %.2f ms   verdict: PASS\n\n",
                test.size(), avg(test), pct(test, 99.0),
                *std::max_element(test.begin(), test.end()));

    // (2) Production, first weekday after release: 10-minute windows, latency measured at the
    // gateway; clients give up after 50 ms (counted as timeouts).
    struct Window
    {
        char const* time;
        double rate;
    };
    std::vector<Window> const day = {{"08:00", 120}, {"10:00", 260}, {"11:30", 340},
                                     {"12:10", 390}, {"12:40", 420}, {"14:00", 300},
                                     {"16:00", 250}, {"19:00", 140}};
    std::printf("== evidence 2: production dashboard, 2026-03-09 (10-minute windows, gateway)\n");
    std::printf("%6s %7s %8s %8s %8s %9s %10s\n", "window", "req/s", "mean ms", "p50 ms", "p99 ms",
                "max ms", "timeouts");
    for (auto const& w : day) {
        std::vector<double> lat;
        double arrival = 0.0;
        double server_free = 0.0;
        std::size_t timeouts = 0;
        while (arrival < 600000.0) {
            double const start = std::max(arrival, server_free);
            server_free = start + rng.exponential(2.0);
            double const l = server_free - arrival;
            lat.push_back(l);
            if (l > 50.0) {
                ++timeouts;
            }
            arrival += rng.exponential(1000.0 / w.rate);
        }
        std::printf("%6s %7.0f %8.2f %8.2f %8.2f %9.1f %10zu\n", w.time, w.rate, avg(lat),
                    pct(lat, 50.0), pct(lat, 99.0), *std::max_element(lat.begin(), lat.end()),
                    timeouts);
    }
    std::printf("\n== evidence 3: on-call page, 2026-03-09 12:47\n");
    std::printf(
        "\"Users report the search page timing out at lunchtime. Release REQ-7 passed. Why?\"\n");
    return 0;
}

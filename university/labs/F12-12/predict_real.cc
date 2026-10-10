// predict_real.cc - F12-12 Listing 2: write the prediction, then measure a REAL queue.
// A generator thread releases requests at exponentially distributed intended times (open loop,
// fixed seed) into a queue; one worker thread serves each request by spinning for exactly
// S = 200 us of wall-clock time. Latency is measured from the intended arrival time to the end
// of service. The M/D/1 prediction R = S + U*S / (2*(1-U)) is printed next to the measurement.
// Timing build: -O2 without sanitizers (see run.sh); "--quick" makes a short run for
// ThreadSanitizer.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
using Us = std::chrono::duration<double, std::micro>;

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

void spin_until(Clock::time_point t)
{
    while (Clock::now() < t) {
    }
}

struct Queue
{
    std::mutex m;
    std::condition_variable cv;
    std::deque<Clock::time_point> items;  // each item is the request's intended arrival time
    bool closed = false;
};

std::vector<double> run(double u, double seconds, double service_us)
{
    Rng rng{static_cast<std::uint64_t>(u * 1000.0)};
    std::vector<Us> offsets;
    for (double t = rng.exponential(service_us / u); t < seconds * 1e6;
         t += rng.exponential(service_us / u)) {
        offsets.emplace_back(t);
    }
    Queue q;
    std::vector<double> lat;
    lat.reserve(offsets.size());
    std::thread worker([&] {
        for (;;) {
            Clock::time_point intended;
            {
                std::unique_lock lock(q.m);
                q.cv.wait(lock, [&] { return q.closed || !q.items.empty(); });
                if (q.items.empty()) {
                    return;
                }
                intended = q.items.front();
                q.items.pop_front();
            }
            spin_until(Clock::now() + std::chrono::duration_cast<Clock::duration>(Us(service_us)));
            lat.push_back(Us(Clock::now() - intended).count());
        }
    });
    auto const t0 = Clock::now() + std::chrono::milliseconds(20);
    for (Us const& off : offsets) {
        auto const when = t0 + std::chrono::duration_cast<Clock::duration>(off);
        spin_until(when);
        {
            std::lock_guard lock(q.m);
            q.items.push_back(when);
        }
        q.cv.notify_one();
    }
    {
        std::lock_guard lock(q.m);
        q.closed = true;
    }
    q.cv.notify_one();
    worker.join();
    return lat;
}

double pct(std::vector<double> v, double p)
{
    auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * static_cast<double>(v.size())));
    std::nth_element(v.begin(), v.begin() + static_cast<long>(rank - 1), v.end());
    return v[rank - 1];
}

int main(int argc, char** argv)
{
    bool const quick = argc > 1 && std::strcmp(argv[1], "--quick") == 0;
    double const s = 200.0;  // service time in microseconds
    double const seconds = quick ? 0.2 : 3.0;
    std::printf("service S = %.0f us (spin), one worker, open-loop arrivals, %.1f s per row\n", s,
                seconds);
    std::printf("%5s %7s %8s | %10s | %10s %9s %9s %12s\n", "U", "req/s", "requests", "model mean",
                "meas. mean", "meas. p50", "meas. p99", "mean - model");
    for (double u : {0.02, 0.30, 0.50, 0.70, 0.85}) {
        std::vector<double> lat = run(u, seconds, s);
        double sum = 0.0;
        for (double x : lat) {
            sum += x;
        }
        double const mean = sum / static_cast<double>(lat.size());
        double const model = s + u * s / (2.0 * (1.0 - u));
        std::printf("%5.2f %7.0f %8zu | %8.0f us | %8.0f us %6.0f us %6.0f us %9.0f us\n", u,
                    u / s * 1e6, lat.size(), model, mean, pct(lat, 50.0), pct(lat, 99.0),
                    mean - model);
    }
    return 0;
}

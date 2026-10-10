// detector.cpp - heartbeat failure detectors: the trade-off between detecting a crash
// quickly and wrongly suspecting a process that is only slow.
// Simulated (an exercise model, not a measured network): a process sends a heartbeat
// every 100 ms; each takes 2 ms plus a small random jitter to arrive, and 2 % of them are
// delayed by an extra 50-400 ms (a congested link). At 20.0 s the process stalls for
// 600 ms (alive, but sending nothing). At 60.0 s it crashes for real.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
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

struct Result
{
    int falseSuspicions = 0;   // episodes in which a live process was suspected
    double detectMs = 0;       // crash -> suspicion
};

// Fixed timeout: suspect when nothing has arrived for `timeout` ms.
Result fixedTimeout(const std::vector<double>& arrivals, double crashMs, double timeout)
{
    Result r;
    for (std::size_t i = 1; i < arrivals.size(); ++i) {
        if (arrivals[i] - arrivals[i - 1] > timeout) {
            ++r.falseSuspicions;
        }
    }
    r.detectMs = arrivals.back() + timeout - crashMs;
    return r;
}

// Adaptive timeout: mean + k * standard deviation of the last 100 gaps between arrivals.
Result adaptive(const std::vector<double>& arrivals, double crashMs, double k, double* lastTimeout)
{
    Result r;
    std::deque<double> window;
    double timeout = 1000.0;                 // until enough history exists
    for (std::size_t i = 1; i < arrivals.size(); ++i) {
        const double gap = arrivals[i] - arrivals[i - 1];
        if (gap > timeout) {
            ++r.falseSuspicions;
        }
        window.push_back(gap);
        if (window.size() > 100) {
            window.pop_front();
        }
        if (window.size() >= 20) {
            double sum = 0, sq = 0;
            for (double g : window) {
                sum += g;
                sq += g * g;
            }
            const double mean = sum / window.size();
            const double var = std::max(0.0, sq / window.size() - mean * mean);
            timeout = mean + k * std::sqrt(var);
        }
    }
    *lastTimeout = timeout;
    r.detectMs = arrivals.back() + timeout - crashMs;
    return r;
}

int main()
{
    Rng rng{11};
    const double crashMs = 60000.0;
    std::vector<double> arrivals;
    for (double sent = 0; sent < crashMs; sent += 100.0) {
        if (sent >= 20000.0 && sent < 20600.0) {
            continue;                                  // the stall: no heartbeats sent
        }
        double delay = 2.0 + 3.0 * rng.uniform();
        if (rng.uniform() < 0.02) {
            delay += 50.0 + 350.0 * rng.uniform();     // a congested moment
        }
        arrivals.push_back(sent + delay);
    }
    std::sort(arrivals.begin(), arrivals.end());       // a late heartbeat can be overtaken
    std::printf("%zu heartbeats arrived before the crash at %.1f s\n\n", arrivals.size(),
                crashMs / 1000);
    std::printf("%-26s %18s %16s\n", "detector", "false suspicions", "detection time");
    for (double t : {150.0, 200.0, 300.0, 500.0, 700.0, 1000.0}) {
        const Result r = fixedTimeout(arrivals, crashMs, t);
        std::printf("fixed timeout %6.0f ms     %18d %13.0f ms\n", t, r.falseSuspicions,
                    r.detectMs);
    }
    for (double k : {2.0, 4.0, 8.0}) {
        double last = 0;
        const Result r = adaptive(arrivals, crashMs, k, &last);
        std::printf("adaptive mean+%.0f*sd        %18d %13.0f ms   (final timeout %.0f ms)\n", k,
                    r.falseSuspicions, r.detectMs, last);
    }
    return 0;
}

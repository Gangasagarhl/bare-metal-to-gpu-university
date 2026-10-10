// loop_1khz.cc - F9-45 Listing 2: a 1 kHz periodic loop on Linux, measured.
//   loop_1khz absolute N   sleep until absolute release times (k * T after the start)
//   loop_1khz relative N   "do the work, then sleep 1 ms" (the common mistake)
// Each cycle does about 200 us of busy work. The program records when each cycle woke up
// and reports lateness (wake-up minus ideal release), the start-to-start period and drift.
// It uses POSIX clock_nanosleep on CLOCK_MONOTONIC (glibc <time.h>).
#include <time.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

constexpr long kPeriodNs = 1'000'000;   // 1 kHz: the rate set in this lab plan
constexpr long kWorkNs = 200'000;       // simulated control work per cycle

long long nowNs()
{
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1'000'000'000LL + t.tv_nsec;
}

timespec toTimespec(long long ns)
{
    timespec t{};
    t.tv_sec = static_cast<time_t>(ns / 1'000'000'000LL);
    t.tv_nsec = static_cast<long>(ns % 1'000'000'000LL);
    return t;
}

void busyWork(long ns)              // stands in for "read sensors, compute, write"
{
    const long long end = nowNs() + ns;
    while (nowNs() < end) {
    }
}

long long percentile(std::vector<long long> v, double p)   // nearest-rank, on a copy
{
    std::sort(v.begin(), v.end());
    std::size_t idx = static_cast<std::size_t>(p / 100.0 * static_cast<double>(v.size()));
    if (idx >= v.size()) { idx = v.size() - 1; }
    return v[idx];
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::puts("usage: loop_1khz absolute|relative cycles");
        return 2;
    }
    const bool absolute = std::strcmp(argv[1], "absolute") == 0;
    const int cycles = std::atoi(argv[2]);
    std::vector<long long> wake(static_cast<std::size_t>(cycles));   // allocated before the loop

    const long long t0 = nowNs() + kPeriodNs;     // first release: one period from now
    long long release = t0;
    for (int k = 0; k < cycles; ++k) {
        if (absolute) {
            const timespec r = toTimespec(release);
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &r, nullptr);
        } else {
            const timespec d = toTimespec(kPeriodNs);
            clock_nanosleep(CLOCK_MONOTONIC, 0, &d, nullptr);
        }
        wake[static_cast<std::size_t>(k)] = nowNs();
        busyWork(kWorkNs);
        release += kPeriodNs;
    }

    std::vector<long long> late, period;
    for (int k = 0; k < cycles; ++k) {
        const long long ideal = t0 + static_cast<long long>(k) * kPeriodNs;
        late.push_back(wake[static_cast<std::size_t>(k)] - ideal);
        if (k > 0) {
            period.push_back(wake[static_cast<std::size_t>(k)] -
                             wake[static_cast<std::size_t>(k - 1)]);
        }
    }
    const double elapsedMs = static_cast<double>(wake.back() - wake.front()) / 1e6;
    const double idealMs = static_cast<double>(cycles - 1) * kPeriodNs / 1e6;
    std::printf("mode %s, %d cycles, period 1000 us, work 200 us per cycle\n", argv[1], cycles);
    std::printf("start-to-start period (us): min %.1f  median %.1f  max %.1f\n",
                *std::min_element(period.begin(), period.end()) / 1e3,
                percentile(period, 50) / 1e3,
                *std::max_element(period.begin(), period.end()) / 1e3);
    if (absolute) {
        std::printf("lateness vs ideal release (us): min %.1f  median %.1f  p99 %.1f  max %.1f\n",
                    *std::min_element(late.begin(), late.end()) / 1e3, percentile(late, 50) / 1e3,
                    percentile(late, 99) / 1e3,
                    *std::max_element(late.begin(), late.end()) / 1e3);
    } else {
        std::printf("lateness of the last cycle vs its ideal release (us): %.1f\n",
                    static_cast<double>(late.back()) / 1e3);
    }
    std::printf("first to last wake-up: %.3f ms (ideal %.3f ms) -> drift %.3f ms,"
                " effective rate %.1f Hz\n",
                elapsedMs, idealMs, elapsedMs - idealMs,
                (cycles - 1) / (elapsedMs / 1e3));
    return 0;
}

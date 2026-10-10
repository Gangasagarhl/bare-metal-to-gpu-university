// latency.cc - F9-66: measure wake-up latency of a periodic thread (a small cyclictest-like tool).
//   latency LOOPS PERIOD_US CPU POLICY [LOAD]
//     POLICY: other | fifo:PRIO        LOAD: none | busy (one busy thread per CPU, normal policy)
//             | hog:PRIO:ON_MS:OFF_MS (a SCHED_FIFO thread on the same CPU, busy ON_MS every OFF_MS)
// The thread sleeps until an absolute time (clock_nanosleep, CLOCK_MONOTONIC, TIMER_ABSTIME),
// then reads the clock: latency = wake time - requested time. It prints a histogram and
// min / median / 99th percentile / max. Numbers are measurements of THIS machine at THIS time.
#include <pthread.h>
#include <sched.h>
#include <time.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

int64_t ns(const timespec& t) { return int64_t{t.tv_sec} * 1'000'000'000 + t.tv_nsec; }
timespec ts(int64_t v) { return timespec{static_cast<time_t>(v / 1'000'000'000), static_cast<long>(v % 1'000'000'000)}; }
int64_t now_ns()
{
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return ns(t);
}

bool pin_and_schedule(int cpu, int policy, int prio, const char* who)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (pthread_setaffinity_np(pthread_self(), sizeof set, &set) != 0) {
        std::fprintf(stderr, "%s: cannot pin to CPU %d\n", who, cpu);
        return false;
    }
    sched_param sp{};
    sp.sched_priority = prio;
    int rc = pthread_setschedparam(pthread_self(), policy, &sp);
    if (rc != 0) {
        std::fprintf(stderr, "%s: cannot set scheduling policy: %s\n", who, std::strerror(rc));
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 5) {
        std::fprintf(stderr, "usage: latency LOOPS PERIOD_US CPU other|fifo:PRIO [none|busy|hog:PRIO:ON:OFF]\n");
        return 2;
    }
    const int loops = std::atoi(argv[1]);
    const int64_t period = int64_t{std::atoi(argv[2])} * 1000;
    const int cpu = std::atoi(argv[3]);
    const std::string policy = argv[4];
    const std::string load = argc > 5 ? argv[5] : "none";
    const int prio = policy.rfind("fifo:", 0) == 0 ? std::atoi(policy.c_str() + 5) : 0;

    std::atomic<bool> stop{false};
    std::vector<std::thread> loaders;
    if (load == "busy") {
        for (unsigned c = 0; c < std::thread::hardware_concurrency(); ++c) {
            loaders.emplace_back([&stop, c] {
                pin_and_schedule(static_cast<int>(c), SCHED_OTHER, 0, "busy");
                volatile uint64_t x = 0;
                while (!stop.load(std::memory_order_relaxed)) {
                    x = x + 1;
                }
            });
        }
    } else if (load.rfind("hog:", 0) == 0) {
        int hp = 0, on_ms = 0, off_ms = 0;
        if (std::sscanf(load.c_str(), "hog:%d:%d:%d", &hp, &on_ms, &off_ms) != 3) {
            std::fprintf(stderr, "bad hog spec\n");
            return 2;
        }
        loaders.emplace_back([&stop, cpu, hp, on_ms, off_ms] {
            pin_and_schedule(cpu, SCHED_FIFO, hp, "hog");
            while (!stop.load(std::memory_order_relaxed)) {
                int64_t until = now_ns() + int64_t{on_ms} * 1'000'000;
                while (now_ns() < until) {
                }
                timespec rest = ts(int64_t{off_ms - on_ms} * 1'000'000);
                clock_nanosleep(CLOCK_MONOTONIC, 0, &rest, nullptr);
            }
        });
    }

    std::vector<int64_t> lat(static_cast<size_t>(loops));
    std::thread measure([&] {
        if (!pin_and_schedule(cpu, prio > 0 ? SCHED_FIFO : SCHED_OTHER, prio, "measure")) {
            std::exit(3);
        }
        int64_t next = now_ns() + period;
        for (int i = 0; i < loops; ++i) {
            timespec t = ts(next);
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr);
            lat[static_cast<size_t>(i)] = now_ns() - next;
            next += period;
        }
    });
    measure.join();
    stop.store(true);
    for (auto& t : loaders) {
        t.join();
    }

    const int64_t edges_us[] = {10, 20, 50, 100, 200, 500, 1000, 2000};
    std::vector<int> bins(std::size(edges_us) + 1, 0);
    for (int64_t v : lat) {
        size_t b = 0;
        while (b < std::size(edges_us) && v / 1000 >= edges_us[b]) {
            ++b;
        }
        ++bins[b];
    }
    std::vector<int64_t> sorted = lat;
    std::sort(sorted.begin(), sorted.end());
    auto pick = [&](double q) { return sorted[static_cast<size_t>(q * static_cast<double>(sorted.size() - 1))] / 1000; };
    std::printf("policy %s, load %s, CPU %d, %d wake-ups every %lld us\n", policy.c_str(), load.c_str(), cpu,
                loops, static_cast<long long>(period / 1000));
    int64_t lo = 0;
    for (size_t b = 0; b < bins.size(); ++b) {
        if (b < std::size(edges_us)) {
            std::printf("  %5lld .. %5lld us : %6d\n", static_cast<long long>(lo), static_cast<long long>(edges_us[b] - 1), bins[b]);
            lo = edges_us[b];
        } else {
            std::printf("  %5lld us and more: %6d\n", static_cast<long long>(lo), bins[b]);
        }
    }
    std::printf("  min %lld us, median %lld us, p99 %lld us, max %lld us\n", static_cast<long long>(pick(0.0)),
                static_cast<long long>(pick(0.5)), static_cast<long long>(pick(0.99)), static_cast<long long>(pick(1.0)));
    return 0;
}

// linux_loop.cc - BR-05 Listing 10: world 3, the same control step as a Linux process.
// A control thread sleeps until absolute release times (period 1 ms) and runs
// br05::Loop::step(); a housekeeping thread does 1.5 periods of work every 10 periods, as
// on the microcontroller. Options choose the scheduling policy, locked memory, background
// load, and (trap 4) a fresh heap allocation inside every control step.
//
//   linux_loop [--policy other|fifo] [--prio N] [--mlock] [--load N] [--alloc] [--steps N]
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <time.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "loop_core.h"

namespace {

constexpr long long kPeriodNs = 1'000'000;           // 1 ms

struct Options {
    bool fifo = false;
    int prio = 80;
    bool lock = false;
    int load = 0;
    bool alloc = false;
    int steps = 1000;
};

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

void sleepUntilNs(long long ns)
{
    const timespec t = toTimespec(ns);
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr) != 0) {
        // interrupted by a signal: sleep again until the same absolute time
    }
}

long minorFaults()                                    // this thread's minor page faults
{
    rusage r{};
    getrusage(RUSAGE_THREAD, &r);
    return r.ru_minflt;
}

void spinNs(long long ns)                             // busy work for a given wall time
{
    const long long end = nowNs() + ns;
    while (nowNs() < end) {
    }
}

std::atomic<bool> stop{false};

void loadThread()                                     // background CPU and cache pressure
{
    std::vector<char> buf(4 << 20, 1);
    unsigned x = 1;
    while (!stop.load(std::memory_order_relaxed)) {
        for (std::size_t i = 0; i < buf.size(); i += 64) {
            x = x * 1103515245u + 12345u;
            buf[i] = static_cast<char>(x);
        }
    }
}

void housekeepingThread(long long start)              // 1.5 periods every 10 periods
{
    for (long long r = 10; !stop.load(); r += 10) {
        sleepUntilNs(start + r * kPeriodNs);
        spinNs(kPeriodNs * 3 / 2);
    }
}

void printStats(const char* what, std::vector<long long> v)
{
    std::sort(v.begin(), v.end());
    const auto at = [&](double q) { return v[static_cast<std::size_t>(q * (v.size() - 1))]; };
    long long sum = 0;
    for (long long x : v) {
        sum += x;
    }
    std::printf("%s (us): min %.1f  p50 %.1f  p99 %.1f  max %.1f  mean %.1f\n", what,
                v.front() / 1e3, at(0.5) / 1e3, at(0.99) / 1e3, v.back() / 1e3,
                static_cast<double>(sum) / v.size() / 1e3);
}

void printProcessSize()
{
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind("VmSize", 0) == 0 || line.rfind("VmRSS", 0) == 0) {
            std::printf("%s\n", line.c_str());
        }
    }
}

}  // namespace

int main(int argc, char** argv)
{
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const bool more = i + 1 < argc;
        if (a == "--policy" && more) {
            o.fifo = std::string(argv[++i]) == "fifo";
        } else if (a == "--prio" && more) {
            o.prio = std::atoi(argv[++i]);
        } else if (a == "--mlock") {
            o.lock = true;
        } else if (a == "--load" && more) {
            o.load = std::atoi(argv[++i]);
        } else if (a == "--alloc") {
            o.alloc = true;
        } else if (a == "--steps" && more) {
            o.steps = std::atoi(argv[++i]);
        } else {
            std::fprintf(stderr, "unknown option %s\n", a.c_str());
            return 2;
        }
    }
    if (o.lock && mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::perror("mlockall");
        return 1;
    }
    if (o.fifo) {
        sched_param p{};
        p.sched_priority = o.prio;
        const int rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &p);
        if (rc != 0) {
            std::fprintf(stderr, "pthread_setschedparam: %s\n", std::strerror(rc));
            return 1;
        }
    }
    std::printf("BR-05 world 3: Linux, policy %s%s, mlockall %s, load threads %d, alloc in step %s\n",
                o.fifo ? "SCHED_FIFO " : "SCHED_OTHER", o.fifo ? std::to_string(o.prio).c_str() : "",
                o.lock ? "yes" : "no", o.load, o.alloc ? "yes" : "no");

    // The helper threads must stay SCHED_OTHER even when main() is SCHED_FIFO, so they are
    // started with an explicit attribute instead of inheriting main()'s policy.
    pthread_attr_t other;
    pthread_attr_init(&other);
    pthread_attr_setinheritsched(&other, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&other, SCHED_OTHER);
    sched_param zero{};
    pthread_attr_setschedparam(&other, &zero);
    std::vector<pthread_t> helpers;
    auto spawn = [&](void* (*fn)(void*), void* arg) {
        pthread_t t{};
        const int rc = pthread_create(&t, &other, fn, arg);
        if (rc != 0) {
            std::fprintf(stderr, "pthread_create: %s\n", std::strerror(rc));
            std::exit(1);
        }
        helpers.push_back(t);
    };
    for (int i = 0; i < o.load; ++i) {
        spawn([](void*) -> void* { loadThread(); return nullptr; }, nullptr);
    }
    const long long start = nowNs() + 20 * kPeriodNs;  // a little time for the helpers to start
    long long startCopy = start;
    spawn([](void* s) -> void* { housekeepingThread(*static_cast<long long*>(s)); return nullptr; },
          &startCopy);

    br05::Loop loop;
    std::vector<long long> lateness, stepTime;
    lateness.reserve(o.steps);                         // no allocation inside the loop ...
    stepTime.reserve(o.steps);
    int bins[6] = {};
    const long long limits[5] = {kPeriodNs / 100, kPeriodNs / 20, kPeriodNs / 10,
                                 kPeriodNs / 4, kPeriodNs / 2};
    int over = 0;
    volatile int32_t sink = 0;
    const long faults0 = minorFaults();
    for (int k = 1; k <= o.steps; ++k) {
        const long long release = start + k * kPeriodNs;
        sleepUntilNs(release);
        const long long woke = nowNs();
        int32_t u = 0;
        if (o.alloc) {                                  // ... unless --alloc asks for it
            std::vector<int32_t> history(64 * 1024);    // 256 KiB, fresh every step
            for (std::size_t i = 0; i < history.size(); i += 1024) {
                history[i] = static_cast<int32_t>(i);
            }
            u = loop.step();
            history[0] = u;
            sink = history[0];
        } else {
            u = loop.step();
        }
        sink = u;                                       // "the actuator write"
        const long long acted = nowNs();
        lateness.push_back(acted - release);
        stepTime.push_back(acted - woke);
        const long long late = acted - release;
        int b = 0;
        while (b < 5 && late >= limits[b]) {
            ++b;
        }
        ++bins[b];
        if (late >= kPeriodNs) {
            ++over;
        }
    }
    const long faults = minorFaults() - faults0;
    stop = true;
    for (pthread_t t : helpers) {
        pthread_join(t, nullptr);
    }
    pthread_attr_destroy(&other);
    (void)sink;

    printStats("actuation lateness", lateness);
    printStats("control step time ", stepTime);
    std::printf("minor page faults of the control thread during the loop: %ld\n", faults);
    static const char* const names[6] = {"  < 1% of period ", "  1-5%           ",
                                         "  5-10%          ", "  10-25%         ",
                                         "  25-50%         ", "  >= 50%         "};
    std::printf("actuation lateness histogram (steps):\n");
    for (int b = 0; b < 6; ++b) {
        std::printf("%s%d\n", names[b], bins[b]);
    }
    const auto mm = std::minmax_element(lateness.begin(), lateness.end());
    long long sum = 0;
    for (long long x : lateness) {
        sum += x;
    }
    std::string world = std::string("linux-") + (o.fifo ? "fifo" : "other") +
                        (o.load > 0 ? "-load" : "-idle") + (o.alloc ? "-alloc" : "");
    std::printf("SUMMARY world=%s unit=ns period=%lld samples=%zu min=%lld max=%lld jitter=%lld "
                "mean=%lld late_by_a_period_or_more=%d checksum=0x%08x\n",
                world.c_str(), kPeriodNs, lateness.size(), *mm.first, *mm.second,
                *mm.second - *mm.first, sum / static_cast<long long>(lateness.size()), over,
                static_cast<unsigned>(loop.checksum));
    printProcessSize();
    return 0;
}

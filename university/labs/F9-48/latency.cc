// latency.cc - F9-48 Listing 1: a small wake-up latency tester, written for this course in
// the style of the classic cyclic test: a thread sleeps until absolute release times and
// records how late it woke up. Optional: real-time policy, locked memory, background load,
// and per-cycle work that touches fresh or reused memory.
//
//   latency --policy other|fifo [--prio N] [--interval US] [--loops N] [--mlock]
//           [--load N] [--work none|fresh|reuse]
#include <malloc.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <time.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Options {
    bool fifo = false;
    int prio = 80;
    long intervalUs = 1000;
    int loops = 5000;
    bool lock = false;
    int load = 0;
    std::string work = "none";
};

long long nowNs()
{
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1'000'000'000LL + t.tv_nsec;
}

long minorFaults()                          // page faults served without disk I/O, this thread
{
    rusage r{};
    getrusage(RUSAGE_THREAD, &r);
    return r.ru_minflt;
}

std::atomic<bool> stopLoad{false};

void loadThread()                           // background CPU and cache pressure, SCHED_OTHER
{
    std::vector<char> buf(4 << 20, 1);
    unsigned x = 1;
    while (!stopLoad.load(std::memory_order_relaxed)) {
        for (std::size_t i = 0; i < buf.size(); i += 64) {
            x = x * 1103515245u + 12345u;
            buf[i] = static_cast<char>(x);
        }
    }
}

constexpr std::size_t kWorkBytes = 256 * 1024;

void doWork(const std::string& kind, std::vector<char>& reused)
{
    if (kind == "fresh") {                  // allocate and touch new memory every cycle
        std::vector<char> fresh(kWorkBytes);
        for (std::size_t i = 0; i < fresh.size(); i += 4096) { fresh[i] = 1; }
        reused[0] = fresh[kWorkBytes / 2];
    } else if (kind == "reuse") {           // the same, prepared, memory every cycle
        for (std::size_t i = 0; i < reused.size(); i += 4096) { reused[i] = 1; }
    }
}

void report(const char* what, std::vector<long long> v)
{
    std::sort(v.begin(), v.end());
    long long sum = 0;
    for (long long x : v) { sum += x; }
    auto pct = [&](double p) {
        return v[std::min(v.size() - 1, static_cast<std::size_t>(p / 100.0 * static_cast<double>(v.size())))];
    };
    std::printf("%s (us): min %.1f avg %.1f p50 %.1f p99 %.1f p99.9 %.1f max %.1f\n", what,
                v.front() / 1e3, static_cast<double>(sum) / static_cast<double>(v.size()) / 1e3,
                pct(50) / 1e3, pct(99) / 1e3, pct(99.9) / 1e3, v.back() / 1e3);
}

void histogram(const std::vector<long long>& v)
{
    const long edges[] = {10, 20, 50, 100, 200, 500, 1000, 2000, 5000};
    long lo = 0;
    std::puts("wake-up latency histogram (# = up to 100 samples):");
    for (long e : edges) {
        const auto n = std::count_if(v.begin(), v.end(),
                                     [&](long long x) { return x >= lo * 1000 && x < e * 1000; });
        std::printf("  %5ld-%-5ld us %6ld %s\n", lo, e - 1, static_cast<long>(n),
                    std::string(static_cast<std::size_t>((n + 99) / 100), '#').c_str());
        lo = e;
    }
    const auto n = std::count_if(v.begin(), v.end(), [&](long long x) { return x >= lo * 1000; });
    std::printf("  >= %-8ld us %6ld %s\n", lo, static_cast<long>(n),
                std::string(static_cast<std::size_t>((n + 99) / 100), '#').c_str());
}

}  // namespace

int main(int argc, char** argv)
{
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const char* next = i + 1 < argc ? argv[i + 1] : "";
        if (a == "--policy") { o.fifo = std::strcmp(next, "fifo") == 0; ++i; }
        else if (a == "--prio") { o.prio = std::atoi(next); ++i; }
        else if (a == "--interval") { o.intervalUs = std::atol(next); ++i; }
        else if (a == "--loops") { o.loops = std::atoi(next); ++i; }
        else if (a == "--mlock") { o.lock = true; }
        else if (a == "--load") { o.load = std::atoi(next); ++i; }
        else if (a == "--work") { o.work = next; ++i; }
        else { std::printf("unknown option %s\n", a.c_str()); return 2; }
    }

    if (o.work == "fresh") {
        // Fix glibc's threshold above which an allocation gets its own fresh pages from the
        // kernel (by default it adapts at run time, which would make the experiment vary).
        mallopt(M_MMAP_THRESHOLD, 128 * 1024);
    }
    if (o.lock && mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::printf("mlockall failed: %s\n", std::strerror(errno));
        return 1;
    }
    std::vector<std::thread> hogs;
    for (int i = 0; i < o.load; ++i) { hogs.emplace_back(loadThread); }

    // Everything the loop needs is allocated and touched before the loop starts.
    std::vector<long long> late(static_cast<std::size_t>(o.loops));
    std::vector<long long> workNs(static_cast<std::size_t>(o.loops));
    std::vector<char> reused(kWorkBytes, 0);
    long faultsInLoop = 0;
    std::string policyText = "SCHED_OTHER";

    std::thread measure([&] {
        if (o.fifo) {
            sched_param sp{};
            sp.sched_priority = o.prio;
            const int err = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
            policyText = err == 0 ? "SCHED_FIFO " + std::to_string(o.prio)
                                  : std::string("SCHED_OTHER (SCHED_FIFO refused: ") +
                                        std::strerror(err) + ")";
        }
        const long long period = o.intervalUs * 1000;
        long long release = nowNs() + 10 * period;
        const long f0 = minorFaults();
        for (int k = 0; k < o.loops; ++k) {
            timespec r{};
            r.tv_sec = static_cast<time_t>(release / 1'000'000'000LL);
            r.tv_nsec = static_cast<long>(release % 1'000'000'000LL);
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &r, nullptr);
            const long long woke = nowNs();
            late[static_cast<std::size_t>(k)] = woke - release;
            doWork(o.work, reused);
            workNs[static_cast<std::size_t>(k)] = nowNs() - woke;
            release += period;
        }
        faultsInLoop = minorFaults() - f0;
    });
    measure.join();
    stopLoad = true;
    for (std::thread& h : hogs) { h.join(); }

    std::printf("policy %s, interval %ld us, %d loops, mlockall %s, load threads %d, work %s\n",
                policyText.c_str(), o.intervalUs, o.loops, o.lock ? "yes" : "no", o.load,
                o.work.c_str());
    report("wake-up latency", late);
    histogram(late);
    if (o.work != "none") {
        report("work time per cycle", workNs);
    }
    std::printf("minor page faults of the measuring thread during the loop: %ld\n", faultsInLoop);
    return 0;
}

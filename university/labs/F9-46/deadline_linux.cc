// deadline_linux.cc - F9-46 Listing 3: EDF in a real kernel. Linux's SCHED_DEADLINE policy
// runs tasks by earliest deadline and reserves (runtime, deadline, period) for each.
//   part C: admission control - ask for more bandwidth than the machine has (run first)
//   part A: a periodic job (runtime 2 ms, deadline 5 ms, period 10 ms) - its start times
//   part B: a job that needs more than its runtime - the kernel throttles it
// struct sched_attr and SCHED_DEADLINE come from the installed Linux UAPI headers
// (<linux/sched/types.h>, <linux/sched.h>); glibc has no wrapper, so syscall() is used.
#include <linux/sched.h>
#include <linux/sched/types.h>
#include <sched.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <latch>
#include <thread>
#include <vector>

namespace {

long long nowNs(clockid_t c = CLOCK_MONOTONIC)
{
    timespec t{};
    clock_gettime(c, &t);
    return t.tv_sec * 1'000'000'000LL + t.tv_nsec;
}

void burnCpu(long long ns)             // consume ns of this thread's own CPU time
{
    const long long end = nowNs(CLOCK_THREAD_CPUTIME_ID) + ns;
    while (nowNs(CLOCK_THREAD_CPUTIME_ID) < end) {
    }
}

int setDeadline(unsigned long long runtimeNs, unsigned long long deadlineNs,
                unsigned long long periodNs)
{
    sched_attr a{};
    a.size = sizeof a;
    a.sched_policy = SCHED_DEADLINE;
    a.sched_runtime = runtimeNs;
    a.sched_deadline = deadlineNs;
    a.sched_period = periodNs;
    return static_cast<int>(syscall(SYS_sched_setattr, 0, &a, 0)) == 0 ? 0 : errno;
}

void partA(int jobs)
{
    std::vector<long long> start;
    start.reserve(static_cast<std::size_t>(jobs));
    int err = 0;
    std::thread t([&] {
        err = setDeadline(2'000'000, 5'000'000, 10'000'000);
        if (err != 0) { return; }
        for (int k = 0; k < jobs; ++k) {
            start.push_back(nowNs());
            burnCpu(1'000'000);          // 1 ms of work, inside the 2 ms runtime
            sched_yield();               // for SCHED_DEADLINE: "this job is done"
        }
    });
    t.join();
    std::printf("part A: SCHED_DEADLINE runtime 2 ms, deadline 5 ms, period 10 ms, %d jobs\n",
                jobs);
    if (err != 0) {
        std::printf("  sched_setattr failed: %s\n", std::strerror(err));
        return;
    }
    std::vector<long long> gap;
    for (std::size_t k = 1; k < start.size(); ++k) { gap.push_back(start[k] - start[k - 1]); }
    std::sort(gap.begin(), gap.end());
    std::printf("  start-to-start (us): min %.1f  median %.1f  max %.1f\n", gap.front() / 1e3,
                gap[gap.size() / 2] / 1e3, gap.back() / 1e3);
}

void partB()
{
    std::printf("part B: runtime 1 ms per 10 ms period, but each job needs 3 ms of CPU\n");
    std::thread t([] {
        const int err = setDeadline(1'000'000, 10'000'000, 10'000'000);
        if (err != 0) {
            std::printf("  sched_setattr failed: %s\n", std::strerror(err));
            return;
        }
        for (int k = 0; k < 4; ++k) {
            const long long w0 = nowNs();
            burnCpu(3'000'000);
            std::printf("  job %d: 3.0 ms of CPU took %.1f ms of wall-clock time\n", k,
                        (nowNs() - w0) / 1e6);
            sched_yield();
        }
    });
    t.join();
}

void partC(int threads)
{
    std::printf("part C: %d threads each ask for runtime 9 ms per 10 ms (bandwidth 0.9)\n",
                threads);
    std::latch tried(threads);           // every thread keeps its reservation until all tried
    std::vector<int> result(static_cast<std::size_t>(threads), 0);
    std::vector<std::thread> pool;
    std::atomic<int> order{0};
    std::vector<int> attempt(static_cast<std::size_t>(threads), 0);
    for (int i = 0; i < threads; ++i) {
        pool.emplace_back([&, i] {
            attempt[static_cast<std::size_t>(i)] = order.fetch_add(1);
            result[static_cast<std::size_t>(i)] = setDeadline(9'000'000, 10'000'000, 10'000'000);
            tried.arrive_and_wait();     // blocked threads keep their reserved bandwidth
        });
    }
    for (std::thread& t : pool) { t.join(); }
    int accepted = 0;
    for (int i = 0; i < threads; ++i) {
        accepted += result[static_cast<std::size_t>(i)] == 0 ? 1 : 0;
    }
    for (int a = 0; a < threads; ++a) {
        for (int i = 0; i < threads; ++i) {
            if (attempt[static_cast<std::size_t>(i)] == a) {
                const int e = result[static_cast<std::size_t>(i)];
                std::printf("  request %d: %s\n", a + 1,
                            e == 0 ? "accepted" : std::strerror(e));
            }
        }
    }
    std::printf("  accepted %d of %d (total requested %.1f CPUs; this machine has %u CPUs)\n",
                accepted, threads, 0.9 * threads, std::thread::hardware_concurrency());
}

}  // namespace

int main(int argc, char** argv)
{
    const bool smoke = argc > 1 && std::strcmp(argv[1], "smoke") == 0;
    partC(smoke ? 2 : 6);     // first, so that no earlier reservation is still counted
    partA(smoke ? 5 : 200);
    if (!smoke) { partB(); }
    return 0;
}

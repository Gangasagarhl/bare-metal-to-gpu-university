// rt_node.cc - F9-49 Listing 2: one "node" with a 1 kHz control callback and a 10 Hz logging
// callback, run on the miniature executor of uexec.h in two configurations:
//   rt_node bad    both callbacks in ONE callback group on ONE executor thread (SCHED_OTHER);
//                  the control callback allocates memory every cycle
//   rt_node fixed  control in its own group on an executor thread with SCHED_FIFO 80 and
//                  locked memory; logging in another group on a SCHED_OTHER thread; control
//                  hands its records to the logger through a preallocated lock-free ring
// Every allocation is counted per thread by replacing the global operator new.
#include <sys/mman.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include "uexec.h"

// ---- allocation counter: every operator new on this thread increments t_allocs ----------
thread_local long t_allocs = 0;
void* operator new(std::size_t n)
{
    ++t_allocs;
    if (void* p = std::malloc(n == 0 ? 1 : n)) { return p; }
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

namespace {

// ---- single-producer single-consumer ring: no locks, no allocation after construction ----
template <typename T, std::size_t N>
class SpscRing {
public:
    bool push(const T& v)                       // producer thread only
    {
        const std::size_t h = head_.load(std::memory_order_relaxed);
        const std::size_t next = (h + 1) % N;
        if (next == tail_.load(std::memory_order_acquire)) { return false; }   // full: drop
        buf_[h] = v;
        head_.store(next, std::memory_order_release);
        return true;
    }
    bool pop(T& v)                              // consumer thread only
    {
        const std::size_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire)) { return false; }      // empty
        v = buf_[t];
        tail_.store((t + 1) % N, std::memory_order_release);
        return true;
    }
private:
    std::array<T, N> buf_{};
    std::atomic<std::size_t> head_{0}, tail_{0};
};

struct Sample { long long time; double setpoint, measured, command; };

// ---- the controlled "motor": first-order speed response, integrated each cycle ----------
struct Motor {
    double speed = 0;
    void step(double command, double dt) { speed += dt * (-speed / 0.05 + 40.0 * command); }
};

struct Pid {
    double kp = 0.08, ki = 2.0, integral = 0;
    double update(double error, double dt)
    {
        integral += error * dt;
        return std::clamp(kp * error + ki * integral, -1.0, 1.0);
    }
};

constexpr long long kControlNs = 1'000'000;   // 1 kHz
constexpr long long kLogNs = 100'000'000;     // 10 Hz
constexpr long long kRunNs = 3'000'000'000;   // 3 s

// The logger's work: format a report and write it to /dev/null (a real system call).
std::size_t formatReport(const std::vector<Sample>& recent)
{
    std::ostringstream out;
    for (int rep = 0; rep < 40; ++rep) {
        for (const Sample& s : recent) {
            out << s.time << ' ' << s.setpoint << ' ' << s.measured << ' ' << s.command << '\n';
        }
    }
    std::ofstream devnull("/dev/null");
    devnull << out.str();
    return out.str().size();
}

void summarise(const uexec::Executor& ex, const char* executorName)
{
    for (std::size_t id = 0; id < ex.names.size(); ++id) {
        std::vector<long long> late, dur;
        int misses = 0;
        for (const uexec::TraceEvent& e : ex.trace) {
            if (e.timer != static_cast<int>(id)) { continue; }
            late.push_back(e.start - e.release);
            dur.push_back(e.end - e.start);
            if (ex.names[id] == "control" && e.end - e.release > kControlNs) { ++misses; }
        }
        if (late.empty()) { continue; }
        std::sort(late.begin(), late.end());
        std::sort(dur.begin(), dur.end());
        const auto p99 = [](const std::vector<long long>& v) { return v[v.size() * 99 / 100]; };
        std::printf("  %-8s on %-9s %-12s runs %5zu  start lateness us: p50 %7.1f p99 %7.1f max"
                    " %7.1f  duration us: p50 %6.1f max %6.1f",
                    ex.names[id].c_str(), executorName, ex.policy.c_str(), late.size(),
                    late[late.size() / 2] / 1e3, p99(late) / 1e3, late.back() / 1e3,
                    dur[dur.size() / 2] / 1e3, dur.back() / 1e3);
        if (ex.names[id] == "control") { std::printf("  missed 1 ms deadline: %d", misses); }
        std::printf("\n");
    }
}

void showWorst(const uexec::Executor& ex)   // the trace just before the latest control start
{
    std::size_t worst = 0;
    long long worstLate = -1;
    for (std::size_t k = 0; k < ex.trace.size(); ++k) {
        const uexec::TraceEvent& e = ex.trace[k];
        if (ex.names[static_cast<std::size_t>(e.timer)] == "control" && e.start - e.release > worstLate) {
            worstLate = e.start - e.release;
            worst = k;
        }
    }
    const long long base = ex.trace[worst].release;
    std::printf("  trace around the worst control start (times in us relative to its release):\n");
    for (std::size_t k = worst >= 4 ? worst - 4 : 0; k <= worst + 1 && k < ex.trace.size(); ++k) {
        const uexec::TraceEvent& e = ex.trace[k];
        std::printf("    %-8s release %+9.1f  start %+9.1f  end %+9.1f%s\n",
                    ex.names[static_cast<std::size_t>(e.timer)].c_str(), (e.release - base) / 1e3,
                    (e.start - base) / 1e3, (e.end - base) / 1e3, k == worst ? "   <- worst" : "");
    }
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string mode = argc > 1 ? argv[1] : "bad";
    const bool fixed = mode == "fixed";
    const long long runNs = mode == "smoke" ? 300'000'000 : kRunNs;

    Motor motor;
    Pid pid;
    double setpoint = 0;
    long tick = 0, controlAllocs = 0;
    std::vector<Sample> history;                        // bad: grows inside the loop
    std::vector<std::string> messages;                  // bad: one string per cycle
    SpscRing<Sample, 512> ring;                         // fixed: preallocated hand-over
    std::vector<Sample> recent;
    recent.reserve(512);
    std::size_t bytes = 0;
    long dropped = 0;

    if (fixed && mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::printf("mlockall failed: %s\n", std::strerror(errno));
    }

    auto control = [&] {
        const long before = t_allocs;
        setpoint = (tick / 500) % 2 == 0 ? 1.0 : 0.5;    // a square wave every 0.5 s
        const double dt = 1e-3;
        const double u = pid.update(setpoint - motor.speed, dt);
        motor.step(u, dt);
        const Sample s{tick, setpoint, motor.speed, u};
        if (fixed) {
            if (!ring.push(s)) { ++dropped; }
        } else {
            history.push_back(s);                                       // may reallocate
            messages.push_back("control: setpoint " + std::to_string(setpoint) +
                               " measured " + std::to_string(motor.speed));   // allocates
        }
        ++tick;
        controlAllocs += t_allocs - before;
    };
    auto logging = [&] {
        recent.clear();
        if (fixed) {
            Sample s{};
            while (ring.pop(s)) { recent.push_back(s); }
        } else {
            const std::size_t n = std::min<std::size_t>(history.size(), 100);
            recent.assign(history.end() - static_cast<long>(n), history.end());
        }
        bytes += formatReport(recent);
    };

    uexec::CallbackGroup controlGroup, logGroup;
    controlGroup.addTimer("control", kControlNs, control);
    if (fixed) {
        logGroup.addTimer("logging", kLogNs, logging);
    } else {
        controlGroup.addTimer("logging", kLogNs, logging);   // same group as the control loop
    }
    uexec::Executor rtExec("rt", fixed ? 80 : 0), logExec("log");
    rtExec.add(controlGroup);
    if (fixed) { logExec.add(logGroup); }

    const long long t0 = uexec::nowNs() + 10'000'000;
    rtExec.start(t0, runNs);
    if (fixed) { logExec.start(t0, runNs); }
    rtExec.join();
    if (fixed) { logExec.join(); }

    std::printf("mode %s: control 1 kHz, logging 10 Hz, %.1f s\n", mode.c_str(), runNs / 1e9);
    summarise(rtExec, "executor1");
    if (fixed) { summarise(logExec, "executor2"); }
    std::printf("  allocations inside control callbacks: %ld in %ld runs\n", controlAllocs, tick);
    if (fixed) { std::printf("  records dropped because the ring was full: %ld\n", dropped); }
    std::printf("  final speed %.3f (setpoint %.1f); logger wrote %zu bytes\n", motor.speed,
                setpoint, bytes);
    showWorst(rtExec);
    return 0;
}

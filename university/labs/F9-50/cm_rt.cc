// cm_rt.cc - F9-50 Listing 5: the uctl stack paced by the clock at 1 kHz (the rate set in this
// lab plan) on a SCHED_FIFO thread with locked memory, measuring its own timing.
// Requirement checked at the end (the lab plan's jitter requirement):
//   R1  start-to-start period within 1000 +/- 100 us for at least 99.9 % of cycles
//   R2  no period longer than 2000 us (no cycle skipped)
//   R3  read + update + write together take less than 200 us in every cycle
//   cm_rt [seconds] [cpu]      cpu: pin the loop thread to this CPU (optional)
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <time.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "controllers.h"
#include "sim_hw.h"
#include "uctl.h"

namespace {

long long nowNs()
{
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1'000'000'000LL + t.tv_nsec;
}

}  // namespace

int main(int argc, char** argv)
{
    const int seconds = argc > 1 ? std::atoi(argv[1]) : 5;
    const int cpu = argc > 2 ? std::atoi(argv[2]) : -1;
    const int cycles = seconds * 1000;
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::printf("mlockall failed: %s (continuing)\n", std::strerror(errno));
    }

    SimWheels hw;
    WheelVelocityPi pi;
    uctl::ControllerManager cm(hw);
    cm.addController(pi);
    cm.configure();                                          // allocation happens here only
    const auto st = hw.exportStates();
    std::vector<long long> start(static_cast<std::size_t>(cycles));
    std::vector<long long> exec(static_cast<std::size_t>(cycles));
    std::string setup;

    std::thread loop([&] {
        sched_param sp{};
        sp.sched_priority = 80;
        const int e = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
        setup = e == 0 ? "SCHED_FIFO 80" : std::string("SCHED_OTHER (") + std::strerror(e) + ")";
        if (cpu >= 0) {
            cpu_set_t set;
            CPU_ZERO(&set);
            CPU_SET(cpu, &set);
            const int a = pthread_setaffinity_np(pthread_self(), sizeof set, &set);
            setup += a == 0 ? ", pinned to CPU " + std::to_string(cpu) : ", pinning refused";
        }
        const double dt = 0.001;
        long long release = nowNs() + 10'000'000;
        for (int k = 0; k < cycles; ++k) {
            timespec r{};
            r.tv_sec = static_cast<time_t>(release / 1'000'000'000LL);
            r.tv_nsec = static_cast<long>(release % 1'000'000'000LL);
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &r, nullptr);
            const long long s = nowNs();
            const double t = k * dt;
            const double target = (k / 1000) % 2 == 0 ? 10.0 : 5.0;
            pi.setTarget(0, target);
            pi.setTarget(1, target);
            cm.cycle(t, dt);                                 // read -> update -> write
            start[static_cast<std::size_t>(k)] = s;
            exec[static_cast<std::size_t>(k)] = nowNs() - s;
            release += 1'000'000;
        }
    });
    loop.join();

    std::vector<long long> period;
    int inBand = 0, skipped = 0;
    for (std::size_t k = 1; k < start.size(); ++k) {
        const long long p = start[k] - start[k - 1];
        period.push_back(p);
        inBand += std::llabs(p - 1'000'000) <= 100'000 ? 1 : 0;
        skipped += p > 2'000'000 ? 1 : 0;
    }
    std::sort(period.begin(), period.end());
    std::vector<long long> ex = exec;
    std::sort(ex.begin(), ex.end());
    const double share = 100.0 * inBand / static_cast<double>(period.size());
    auto at = [](const std::vector<long long>& v, double p) {
        return v[std::min(v.size() - 1, static_cast<std::size_t>(p / 100.0 * static_cast<double>(v.size())))] / 1e3;
    };
    std::printf("loop thread: %s; mlockall; %d cycles at 1 kHz\n", setup.c_str(), cycles);
    std::printf("period (us): min %.1f p0.1 %.1f p50 %.1f p99.9 %.1f max %.1f\n",
                period.front() / 1e3, at(period, 0.1), at(period, 50), at(period, 99.9),
                period.back() / 1e3);
    std::printf("read+update+write (us): p50 %.1f p99.9 %.1f max %.1f\n", at(ex, 50), at(ex, 99.9),
                ex.back() / 1e3);
    std::printf("R1 periods within 1000 +/- 100 us: %.2f %% (need >= 99.9 %%) -> %s\n", share,
                share >= 99.9 ? "PASS" : "FAIL");
    std::printf("R2 periods longer than 2000 us: %d -> %s\n", skipped, skipped == 0 ? "PASS" : "FAIL");
    std::printf("R3 longest read+update+write: %.1f us (need < 200) -> %s\n", ex.back() / 1e3,
                ex.back() < 200'000 ? "PASS" : "FAIL");
    std::printf("final wheel speeds %.2f and %.2f rad/s\n", *st[1].value, *st[3].value);
    return 0;
}

// freeze.cc - F9-70: reproduce "the robot froze for two seconds" on the build machine.
//   freeze CONTROL_CPU CAMERA_CPU
// Two real threads, both SCHED_FIFO:
//   control (priority 80): a 1 ms loop that records every cycle in a flight recorder;
//   camera  (priority 90): a stand-in for a USB camera driver's error handling: 1 s after the
//           start it "resets the device" and keeps its CPU busy for 2 s, writing log lines.
// The camera thread is the university's simulation, not the Linux USB driver: its log lines are
// written by this program in a kernel-log-like layout. After 4 s the program prints the camera
// log, the gaps in the control loop found in the flight recorder, and a lateness histogram.
#include <pthread.h>
#include <sched.h>
#include <time.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "flightrec.h"

namespace {

int64_t now_ns()
{
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return int64_t{t.tv_sec} * 1'000'000'000 + t.tv_nsec;
}
void sleep_until_ns(int64_t when)
{
    timespec t{static_cast<time_t>(when / 1'000'000'000), static_cast<long>(when % 1'000'000'000)};
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr);
}
bool rt(int cpu, int prio)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    sched_param sp{};
    sp.sched_priority = prio;
    return pthread_setaffinity_np(pthread_self(), sizeof set, &set) == 0 &&
           pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp) == 0;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: freeze CONTROL_CPU CAMERA_CPU\n");
        return 2;
    }
    const int control_cpu = std::atoi(argv[1]);
    const int camera_cpu = std::atoi(argv[2]);
    auto rec = std::make_unique<FlightRecorder<8192>>();   // 8192 cycles = the last 8 s at 1 ms
    std::vector<std::string> klog;                          // written by the camera thread only
    std::atomic<bool> stop{false};
    std::atomic<int> setup_errors{0};
    const int64_t t0 = now_ns();
    auto stamp = [t0](int64_t t) {
        char b[32];
        std::snprintf(b, sizeof b, "[%5lld.%06lld]", static_cast<long long>((t - t0) / 1'000'000'000),
                      static_cast<long long>((t - t0) % 1'000'000'000 / 1000));
        return std::string(b);
    };

    std::thread camera([&] {
        if (!rt(camera_cpu, 90)) {
            ++setup_errors;
            return;
        }
        sleep_until_ns(t0 + 1'000'000'000);
        klog.push_back(stamp(now_ns()) + " cam0: no frame for 500 ms, resetting device (sim)");
        int64_t until = now_ns() + 2'000'000'000;
        while (now_ns() < until) {        // the reset path keeps the CPU busy (retries, polling)
        }
        klog.push_back(stamp(now_ns()) + " cam0: reset complete, streaming again (sim)");
    });
    std::thread control([&] {
        if (!rt(control_cpu, 80)) {
            ++setup_errors;
            return;
        }
        int64_t next = now_ns() + 1'000'000;
        for (uint32_t cycle = 0; !stop.load(std::memory_order_relaxed); ++cycle) {
            sleep_until_ns(next);
            int64_t now = now_ns();
            rec->push(Record{now, cycle, static_cast<int32_t>((now - next) / 1000), Ev::Cycle});
            next += 1'000'000;
            if (now > next) {             // overran: skip the missed releases, keep the phase
                next += (now - next) / 1'000'000 * 1'000'000 + 1'000'000;
            }
        }
    });
    std::this_thread::sleep_for(std::chrono::seconds(4));
    stop.store(true);
    control.join();
    camera.join();
    if (setup_errors.load() != 0) {
        std::printf("could not set SCHED_FIFO or the CPU affinity (needs the right privileges)\n");
        return 3;
    }

    std::printf("== camera log (control on CPU %d, camera on CPU %d)\n", control_cpu, camera_cpu);
    for (const auto& line : klog) {
        std::printf("%s\n", line.c_str());
    }
    std::printf("== flight recorder: %llu records written, %llu overwritten\n",
                static_cast<unsigned long long>(rec->written()), static_cast<unsigned long long>(rec->overwritten()));
    std::printf("== gaps between consecutive control cycles longer than 5 ms\n");
    int64_t prev = -1;
    uint32_t prev_cycle = 0;
    int gaps = 0;
    int64_t worst = 0;
    int bins[5] = {0, 0, 0, 0, 0};       // lateness < 0.1 ms, < 1 ms, < 10 ms, < 100 ms, more
    rec->for_each([&](const Record& r) {
        int64_t late = r.value;
        bins[late < 100 ? 0 : late < 1000 ? 1 : late < 10000 ? 2 : late < 100000 ? 3 : 4]++;
        if (prev >= 0 && r.t_ns - prev > 5'000'000) {
            ++gaps;
            std::printf("%s cycle %u -> %s cycle %u: gap %lld ms\n", stamp(prev).c_str(), prev_cycle,
                        stamp(r.t_ns).c_str(), r.cycle, static_cast<long long>((r.t_ns - prev) / 1'000'000));
        }
        worst = r.t_ns - prev > worst && prev >= 0 ? r.t_ns - prev : worst;
        prev = r.t_ns;
        prev_cycle = r.cycle;
    });
    std::printf("gaps: %d; longest interval between cycles: %lld ms\n", gaps, static_cast<long long>(worst / 1'000'000));
    std::printf("lateness histogram: <0.1 ms %d | <1 ms %d | <10 ms %d | <100 ms %d | >=100 ms %d\n",
                bins[0], bins[1], bins[2], bins[3], bins[4]);
    return 0;
}

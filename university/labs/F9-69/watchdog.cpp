// watchdog.cpp - F9-69: a software watchdog with real threads on the build machine.
// The control thread runs every 10 ms and "kicks" by storing the time of its last good cycle.
// In cycle 30 it hangs for 300 ms (a stand-in for a blocked driver call). The watchdog thread
// checks every 5 ms; when the last kick is older than 50 ms it requests the safe state.
// The bound to check: detection happens no later than timeout + one check period (+ the
// scheduling delay of the watchdog thread itself, which this run measures).
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

using Clock = std::chrono::steady_clock;
using std::chrono::milliseconds;

namespace {
const Clock::time_point kStart = Clock::now();
int64_t now_us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - kStart).count();
}
}  // namespace

int main()
{
    constexpr int64_t kTimeoutUs = 50'000;
    constexpr int64_t kCheckUs = 5'000;
    std::atomic<int64_t> last_kick_us{now_us()};
    std::atomic<bool> safe_state{false};
    std::atomic<bool> stop{false};
    int64_t detected_at = -1;
    int64_t kick_seen = -1;

    std::thread control([&] {
        auto next = Clock::now();
        for (int cycle = 0; cycle < 60 && !safe_state.load(); ++cycle) {
            next += milliseconds(10);
            std::this_thread::sleep_until(next);
            if (cycle == 30) {
                std::this_thread::sleep_for(milliseconds(300));   // the hang
            }
            if (!safe_state.load()) {
                last_kick_us.store(now_us());
            }
        }
    });
    std::thread watchdog([&] {
        while (!stop.load()) {
            std::this_thread::sleep_for(std::chrono::microseconds(kCheckUs));
            int64_t now = now_us();
            int64_t kick = last_kick_us.load();
            if (now - kick > kTimeoutUs && !safe_state.load()) {
                safe_state.store(true);          // on a robot: cut the drive enable here
                detected_at = now;
                kick_seen = kick;
            }
        }
    });
    control.join();
    stop.store(true);
    watchdog.join();

    if (detected_at < 0) {
        std::cout << "watchdog never fired\n";
        return 1;
    }
    int64_t gap = detected_at - kick_seen;
    std::cout << "timeout " << kTimeoutUs / 1000 << " ms, check period " << kCheckUs / 1000 << " ms\n";
    std::cout << "last kick at " << kick_seen / 1000 << " ms, safe state requested at "
              << detected_at / 1000 << " ms\n";
    std::cout << "detection gap " << gap << " us; bound timeout + check period = "
              << kTimeoutUs + kCheckUs << " us; extra scheduling delay "
              << (gap > kTimeoutUs + kCheckUs ? gap - kTimeoutUs - kCheckUs : 0) << " us\n";
    std::cout << "control thread stopped kicking after the safe state: "
              << (last_kick_us.load() == kick_seen ? "yes" : "no") << '\n';
    return 0;
}

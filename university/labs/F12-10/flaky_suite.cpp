// flaky_suite.cpp - three tests with the two most common shapes of flakiness, and one fixed
// test, each run 200 times. A test is flaky if the same code both passes and fails.
//   async_wait_sleep : waits a fixed 2 ms for a worker whose duration varies (0-4 ms)
//   order_dependent  : passes or fails depending on which test ran before it
//   async_wait_fixed : waits for the worker's signal, with a generous deadline
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <thread>

namespace {

std::mt19937 g_work_rng(42);  // varies the worker's duration from run to run

int work_us()
{
    return static_cast<int>(g_work_rng() % 4001);  // 0 .. 4000 microseconds
}

bool async_wait_sleep()
{
    bool done = false;
    std::mutex m;
    const int us = work_us();
    std::thread worker([&] {
        std::this_thread::sleep_for(std::chrono::microseconds(us));  // the "I/O"
        std::lock_guard<std::mutex> lock(m);
        done = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2));  // "should be enough"
    bool ok;
    {
        std::lock_guard<std::mutex> lock(m);
        ok = done;
    }
    worker.join();
    return ok;
}

bool async_wait_fixed()
{
    bool done = false;
    std::mutex m;
    std::condition_variable cv;
    const int us = work_us();
    std::thread worker([&] {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
        {
            std::lock_guard<std::mutex> lock(m);
            done = true;
        }
        cv.notify_one();
    });
    bool ok;
    {
        std::unique_lock<std::mutex> lock(m);
        // wait for the event itself, with a deadline far above the worker's time
        ok = cv.wait_for(lock, std::chrono::seconds(5), [&] { return done; });
    }
    worker.join();
    return ok;
}

// Shared state between tests: a cache of sensor calibrations, global like many real caches.
std::map<std::string, double>& cache()
{
    static std::map<std::string, double> c;
    return c;
}

bool fills_cache()
{
    cache()["lidar"] = 0.02;
    return cache().size() == 1;
}

bool order_dependent()  // assumes it starts with an empty cache
{
    cache()["camera"] = 0.5;
    const bool ok = cache().size() == 1;
    cache().clear();
    return ok;
}

}  // namespace

int main()
{
    const int runs = 200;
    int fail_sleep = 0, fail_order = 0, fail_fixed = 0;
    for (int run = 1; run <= runs; ++run) {
        std::mt19937 order_rng(static_cast<unsigned>(run));  // test order shuffled per run
        const bool fill_first = order_rng() % 2 == 0;
        cache().clear();
        if (fill_first) {
            fills_cache();
            fail_order += order_dependent() ? 0 : 1;
        } else {
            fail_order += order_dependent() ? 0 : 1;
            fills_cache();
        }
        fail_sleep += async_wait_sleep() ? 0 : 1;
        fail_fixed += async_wait_fixed() ? 0 : 1;
    }
    std::printf("test               failed runs (of %d)  verdict\n", runs);
    auto line = [&](const char* name, int f) {
        std::printf("%-18s %10d             %s\n", name, f,
                    f == 0 ? "stable pass" : (f == runs ? "stable fail" : "FLAKY"));
    };
    line("async_wait_sleep", fail_sleep);
    line("order_dependent", fail_order);
    line("async_wait_fixed", fail_fixed);
    return fail_fixed == 0 ? 0 : 1;
}

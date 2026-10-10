// group_threads.cpp - real threads: what a callback group promises (F9-39).
// Two worker threads take callbacks from one queue, like a two-thread executor.
// Exclusive group: a mutex per group, so its callbacks never overlap and may share plain data.
// Reentrant group: no lock, so the callbacks must protect shared data themselves (an atomic here).
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

struct CallbackGroup {
    bool exclusive;
    std::mutex m;                              // used only when exclusive
};

class TwoThreadExecutor {
public:
    void add(CallbackGroup& g, std::function<void()> cb) { work_.push_back({&g, std::move(cb)}); }
    void spin()                                // runs until the queue is empty
    {
        std::vector<std::jthread> threads;
        for (int i = 0; i < 2; ++i) threads.emplace_back([this] { worker(); });
    }                                          // jthread joins here
private:
    struct Item { CallbackGroup* g; std::function<void()> cb; };
    void worker()
    {
        while (true) {
            Item item;
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                if (work_.empty()) return;
                item = std::move(work_.front());
                work_.pop_front();
            }
            if (item.g->exclusive) {
                std::lock_guard<std::mutex> lock(item.g->m);
                item.cb();
            } else {
                item.cb();
            }
        }
    }
    std::mutex queueMutex_;
    std::deque<Item> work_;
};

int main()
{
    CallbackGroup exclusive{true, {}};
    CallbackGroup reentrant{false, {}};
    long plainCount = 0;                       // touched only by the exclusive group's callbacks
    std::atomic<int> inside{0}, maxInside{0};
    std::atomic<long> atomicCount{0};          // touched by the reentrant group's callbacks

    TwoThreadExecutor ex;
    for (int i = 0; i < 200; ++i) {
        ex.add(exclusive, [&] {
            int now = ++inside;
            int seen = maxInside.load();
            while (now > seen && !maxInside.compare_exchange_weak(seen, now)) {}
            for (int k = 0; k < 1000; ++k) ++plainCount;
            --inside;
        });
        ex.add(reentrant, [&] {
            for (int k = 0; k < 1000; ++k) atomicCount.fetch_add(1, std::memory_order_relaxed);
        });
    }
    ex.spin();
    std::printf("exclusive group: plain counter = %ld (expected 200000), at most %d callback(s) inside at once\n",
                plainCount, maxInside.load());
    std::printf("reentrant group: atomic counter = %ld (expected 200000)\n", atomicCount.load());
    return (plainCount == 200000 && atomicCount.load() == 200000 && maxInside.load() == 1) ? 0 : 1;
}

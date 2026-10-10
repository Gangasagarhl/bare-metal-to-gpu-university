// Listing 3 (F2-39): where relaxed is enough: a statistics counter that nobody uses to publish data.
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

std::atomic<long> served{0};

int main()
{
    std::vector<std::thread> waiters;
    for (int w = 0; w < 4; ++w) {
        waiters.emplace_back([] {
            for (int i = 0; i < 100'000; ++i) {
                served.fetch_add(1, std::memory_order_relaxed);  // atomic, orders nothing else
            }
        });
    }
    for (std::thread& t : waiters) {
        t.join();  // join() synchronises: every increment happens-before the read below
    }
    std::cout << "dishes served: " << served.load(std::memory_order_relaxed) << " (expected 400000)\n";
    return 0;
}

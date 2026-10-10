// Listing 4 (F2-38): a spinlock made from std::atomic_flag, protecting a plain counter.
#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class Spinlock
{
public:
    void lock()
    {
        while (flag_.test_and_set(std::memory_order_acquire)) {  // was it already set? keep trying
            while (flag_.test(std::memory_order_relaxed)) {      // wait by reading, not writing
            }
        }
    }

    void unlock()
    {
        flag_.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag_;  // clear = unlocked
};

int main()
{
    Spinlock spin;
    long counter = 0;  // plain long, protected by spin
    std::vector<std::thread> ts;
    for (int t = 0; t < 4; ++t) {
        ts.emplace_back([&spin, &counter] {
            for (int i = 0; i < 100'000; ++i) {
                std::lock_guard<Spinlock> guard(spin);  // works with any type that has lock()/unlock()
                ++counter;
            }
        });
    }
    for (std::thread& t : ts) {
        t.join();
    }
    std::cout << "counter = " << counter << " (expected 400000)\n";
    return 0;
}

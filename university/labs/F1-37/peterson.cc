// Peterson's lock for two threads, written with C++ atomics in two ways. Each thread enters
// the critical section many times and adds 1 to a shared counter there with a separate load
// and store. If the lock really excludes the other thread, no addition is ever lost.
// Counts are observations on the machine that ran this program, not guarantees.
#include <atomic>
#include <cstdio>
#include <thread>

constexpr int kEntries = 1'000'000;

template <std::memory_order StoreOrder, std::memory_order LoadOrder>
struct Peterson
{
    std::atomic<bool> wants[2] = {false, false};
    std::atomic<int> turn{0};
    std::atomic<int> counter{0};      // protected by the lock (relaxed: the lock must order it)

    void lock(int me)
    {
        const int other = 1 - me;
        wants[me].store(true, StoreOrder);
        turn.store(other, StoreOrder);
        while (wants[other].load(LoadOrder) && turn.load(LoadOrder) == other) {
            // wait: the other thread wants in and it is its turn
        }
    }

    void unlock(int me) { wants[me].store(false, StoreOrder); }

    void run(int me)
    {
        for (int i = 0; i < kEntries; ++i) {
            lock(me);
            const int v = counter.load(std::memory_order_relaxed);   // read ...
            counter.store(v + 1, std::memory_order_relaxed);          // ... then write back
            unlock(me);
        }
    }
};

template <std::memory_order StoreOrder, std::memory_order LoadOrder>
void experiment(const char* name)
{
    Peterson<StoreOrder, LoadOrder> p;
    std::thread a([&p] { p.run(0); });
    std::thread b([&p] { p.run(1); });
    a.join();
    b.join();
    const int lost = 2 * kEntries - p.counter.load();
    std::printf("%-8s counter = %d, expected %d, lost additions: %d\n", name, p.counter.load(),
                2 * kEntries, lost);
}

int main()
{
    experiment<std::memory_order_release, std::memory_order_acquire>("rel/acq");
    experiment<std::memory_order_seq_cst, std::memory_order_seq_cst>("seq_cst");
    return 0;
}

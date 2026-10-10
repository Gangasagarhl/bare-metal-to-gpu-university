// scale.cc - F12-15 Listing 3: measure throughput against concurrency on THIS machine.
// N threads each repeat one "request": some private work, then a short update of shared state
// under one mutex (the serial part). Each N runs for 1 s; the total requests per second X(N)
// is printed as "N X" lines that usl.cpp (Listing 2) reads. Timing build: -O2 (see run.sh).
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

std::uint64_t private_work(std::uint64_t h)
{
    for (int i = 0; i < 300; ++i) {
        h = (h ^ (h >> 29)) * 0xBF58476D1CE4E5B9ULL + static_cast<std::uint64_t>(i);
    }
    return h;
}

int main()
{
    std::mutex shared_mu;
    std::vector<std::uint64_t> shared(64, 0);  // shared state: one small table
    std::printf("# measured on this machine: %u hardware threads reported\n",
                std::thread::hardware_concurrency());
    std::printf("# N X   (X = requests per second, all threads together, 1 s per row)\n");
    for (int n : {1, 2, 3, 4, 6, 8}) {
        std::atomic<bool> go{false};
        std::atomic<bool> stop{false};
        std::vector<std::uint64_t> done(static_cast<std::size_t>(n), 0);
        std::vector<std::thread> threads;
        for (int t = 0; t < n; ++t) {
            threads.emplace_back([&, t] {
                while (!go.load()) {
                }
                std::uint64_t h = static_cast<std::uint64_t>(t) + 1;
                std::uint64_t count = 0;
                while (!stop.load(std::memory_order_relaxed)) {
                    h = private_work(h);
                    {
                        std::lock_guard lock(shared_mu);
                        shared[h % shared.size()] += 1;
                    }
                    ++count;
                }
                done[static_cast<std::size_t>(t)] = count;
            });
        }
        auto const t0 = Clock::now();
        go = true;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        stop = true;
        for (std::thread& th : threads) {
            th.join();
        }
        double const seconds = std::chrono::duration<double>(Clock::now() - t0).count();
        std::uint64_t total = 0;
        for (std::uint64_t c : done) {
            total += c;
        }
        std::printf("%d %.1f\n", n, static_cast<double>(total) / seconds);
    }
    return 0;
}

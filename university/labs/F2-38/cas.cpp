// Listing 2 (F2-38): compare_exchange, the "change it only if it is still what I saw" operation,
// first step by step in one thread, then as a retry loop that keeps a running maximum.
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

// Raises `best` to `candidate` if candidate is larger; safe when many threads call it.
void updateMax(std::atomic<int>& best, int candidate)
{
    int current = best.load();
    while (candidate > current && !best.compare_exchange_weak(current, candidate)) {
        // on failure compare_exchange_weak has loaded the newer value into `current`; try again
    }
}

int main()
{
    std::atomic<int> value{10};
    int expected = 10;
    bool ok = value.compare_exchange_strong(expected, 20);
    std::cout << "CAS(expect 10 -> 20): " << (ok ? "succeeded" : "failed") << ", value " << value.load()
              << ", expected now " << expected << '\n';
    expected = 10;  // a stale belief: the value is really 20
    ok = value.compare_exchange_strong(expected, 30);
    std::cout << "CAS(expect 10 -> 30): " << (ok ? "succeeded" : "failed") << ", value " << value.load()
              << ", expected now " << expected << '\n';

    std::atomic<int> best{0};
    std::vector<std::thread> judges;
    for (int t = 0; t < 4; ++t) {
        judges.emplace_back([&best, t] {
            for (int score = t; score < 100'000; score += 4) {
                updateMax(best, score);
            }
        });
    }
    for (std::thread& j : judges) {
        j.join();
    }
    std::cout << "highest score seen by four threads: " << best.load() << '\n';
    return 0;
}

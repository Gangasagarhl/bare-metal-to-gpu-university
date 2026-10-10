// Listing 1 (F2-38): the basic operations of std::atomic, and a counter shared by four threads.
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main()
{
    std::atomic<long> tally{0};

    tally.store(5);                          // atomic write
    const long seen = tally.load();          // atomic read
    const long before = tally.fetch_add(3);  // add 3, return the value before
    const long old = tally.exchange(100);    // write 100, return the value before
    std::cout << "load " << seen << ", fetch_add returned " << before << ", exchange returned " << old
              << ", now " << tally.load() << '\n';

    tally = 0;
    std::vector<std::thread> clickers;
    for (int t = 0; t < 4; ++t) {
        clickers.emplace_back([&tally] {
            for (int i = 0; i < 250'000; ++i) {
                tally.fetch_add(1);  // one indivisible read-modify-write: no lost clicks
            }
        });
    }
    for (std::thread& c : clickers) {
        c.join();
    }
    std::cout << "4 threads x 250000 clicks = " << tally.load() << '\n';
    return 0;
}

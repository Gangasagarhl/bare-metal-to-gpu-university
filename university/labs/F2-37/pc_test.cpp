// Listing 2 (F2-37): the curriculum B10 acceptance test, in user space:
// "Producer/consumer test with bounded buffer: 10 million items, no loss, no duplicates".
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "bounded_queue.hpp"

int main(int argc, char** argv)
{
    const std::uint32_t items = argc > 1 ? static_cast<std::uint32_t>(std::atol(argv[1])) : 10'000'000;
    const unsigned producers = 2;
    const unsigned consumers = 2;
    BoundedQueue<std::uint32_t> queue(1024);
    std::vector<std::uint8_t> seen(items, 0);           // seen[v]: how many times v was received
    std::vector<std::vector<std::uint32_t>> received(consumers);

    std::vector<std::thread> threads;
    for (unsigned p = 0; p < producers; ++p) {
        threads.emplace_back([&queue, p, producers, items] {
            for (std::uint32_t v = p; v < items; v += producers) {  // producer p sends p, p+2, p+4, ...
                queue.push(v);
            }
        });
    }
    for (unsigned c = 0; c < consumers; ++c) {
        threads.emplace_back([&queue, &received, c] {
            while (auto v = queue.pop()) {
                received[c].push_back(*v);  // each consumer writes only its own vector
            }
        });
    }
    for (unsigned p = 0; p < producers; ++p) {
        threads[p].join();
    }
    queue.close();  // all items are in; let the consumers finish
    for (unsigned c = 0; c < consumers; ++c) {
        threads[producers + c].join();
    }

    std::uint64_t total = 0;
    for (unsigned c = 0; c < consumers; ++c) {
        std::cout << "consumer " << c << " received " << received[c].size() << " items\n";
        for (std::uint32_t v : received[c]) {
            ++seen[v];
            ++total;
        }
    }
    std::uint64_t lost = 0;
    std::uint64_t duplicated = 0;
    for (std::uint8_t s : seen) {
        lost += (s == 0) ? 1 : 0;
        duplicated += (s > 1) ? 1 : 0;
    }
    std::cout << "items sent: " << items << ", received: " << total << ", lost: " << lost
              << ", duplicated: " << duplicated << '\n';
    std::cout << "producer waits (queue full): " << queue.pushWaits()
              << ", consumer waits (queue empty): " << queue.popWaits() << '\n';
    const bool pass = total == items && lost == 0 && duplicated == 0;
    std::cout << (pass ? "PASS" : "FAIL") << '\n';
    return pass ? 0 : 1;
}

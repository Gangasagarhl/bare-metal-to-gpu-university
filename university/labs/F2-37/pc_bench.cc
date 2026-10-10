// Measurement (F2-37): throughput of BoundedQueue and how often threads had to wait,
// for several capacities and thread counts. Built with -O2, no sanitizers, by run.sh.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "bounded_queue.hpp"

struct Result
{
    double itemsPerSecond;
    long pushWaits;
    long popWaits;
};

Result runOnce(std::size_t capacity, unsigned pairs, std::uint32_t items)
{
    BoundedQueue<std::uint32_t> q(capacity);
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> ts;
    for (unsigned p = 0; p < pairs; ++p) {
        ts.emplace_back([&q, p, pairs, items] {
            for (std::uint32_t v = p; v < items; v += pairs) {
                q.push(v);
            }
        });
    }
    std::vector<std::thread> cs;
    std::vector<std::uint64_t> sums(pairs, 0);
    for (unsigned c = 0; c < pairs; ++c) {
        cs.emplace_back([&q, &sums, c] {
            while (auto v = q.pop()) {
                sums[c] += *v;
            }
        });
    }
    for (auto& t : ts) {
        t.join();
    }
    q.close();
    for (auto& c : cs) {
        c.join();
    }
    const auto t1 = std::chrono::steady_clock::now();
    std::uint64_t sum = 0;
    for (auto s : sums) {
        sum += s;
    }
    const std::uint64_t expect = static_cast<std::uint64_t>(items) * (items - 1) / 2;
    if (sum != expect) {
        std::cout << "CHECKSUM MISMATCH\n";
    }
    return {items / std::chrono::duration<double>(t1 - t0).count(), q.pushWaits(), q.popWaits()};
}

int main()
{
    const std::uint32_t items = 400'000;
    std::cout << items << " items per run; median of 3 runs; P producers and P consumers\n";
    for (unsigned pairs : {1u, 2u, 4u}) {
        for (std::size_t capacity : {std::size_t{1}, std::size_t{16}, std::size_t{1024}}) {
            std::vector<Result> rs;
            for (int r = 0; r < 3; ++r) {
                rs.push_back(runOnce(capacity, pairs, items));
            }
            std::sort(rs.begin(), rs.end(),
                      [](const Result& a, const Result& b) { return a.itemsPerSecond < b.itemsPerSecond; });
            const Result& m = rs[1];
            std::cout << "P=" << pairs << " capacity " << capacity << ": "
                      << static_cast<long>(m.itemsPerSecond / 1000) << " thousand items/s, producer waits "
                      << m.pushWaits << ", consumer waits " << m.popWaits << '\n';
        }
    }
    return 0;
}

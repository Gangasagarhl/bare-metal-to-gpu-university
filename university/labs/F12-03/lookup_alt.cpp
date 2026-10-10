// lookup_alt.cpp - SE301 F12-03: measure two alternatives instead of arguing about them.
// A driver keeps a table "device id -> handler index". Alternative A: a std::vector of
// pairs searched from the front. Alternative B: a std::unordered_map. Which is faster
// depends on the number of entries N, so we measure several N on THIS machine.
// Times are per lookup, the minimum of 5 repetitions (guide AH-23: a measurement, not a spec).
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <utility>
#include <vector>

using Clock = std::chrono::steady_clock;

static std::uint32_t idOf(std::uint32_t i) { return i * 2654435761u; }   // spread-out ids

int main()
{
    const std::size_t sizes[] = {4, 16, 64, 256, 4096};
    std::printf("%6s %10s %16s %16s\n", "N", "lookups", "A vector ns", "B hash map ns");
    for (std::size_t n : sizes) {
        std::vector<std::pair<std::uint32_t, int>> vec;
        std::unordered_map<std::uint32_t, int> map;
        for (std::uint32_t i = 0; i < n; ++i) {
            vec.emplace_back(idOf(i), static_cast<int>(i));
            map.emplace(idOf(i), static_cast<int>(i));
        }
        const std::size_t lookups = std::max<std::size_t>(2000, 2'000'000 / n);
        double bestA = 1e30;
        double bestB = 1e30;
        long long check = 0;
        for (int rep = 0; rep < 5; ++rep) {
            auto t0 = Clock::now();
            for (std::size_t k = 0; k < lookups; ++k) {
                const std::uint32_t key = idOf(static_cast<std::uint32_t>((k * 7) % n));
                const auto it = std::find_if(vec.begin(), vec.end(),
                                             [key](const auto& p) { return p.first == key; });
                check += it->second;
            }
            auto t1 = Clock::now();
            for (std::size_t k = 0; k < lookups; ++k) {
                const std::uint32_t key = idOf(static_cast<std::uint32_t>((k * 7) % n));
                check += map.find(key)->second;
            }
            auto t2 = Clock::now();
            bestA = std::min(bestA, std::chrono::duration<double, std::nano>(t1 - t0).count());
            bestB = std::min(bestB, std::chrono::duration<double, std::nano>(t2 - t1).count());
        }
        std::printf("%6zu %10zu %16.1f %16.1f   (check %lld)\n", n, lookups,
                    bestA / static_cast<double>(lookups), bestB / static_cast<double>(lookups),
                    check % 1000);
    }
    return 0;
}

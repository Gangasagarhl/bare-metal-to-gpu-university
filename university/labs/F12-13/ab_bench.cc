// ab_bench.cc - F12-13 Listing 3: a real A/B benchmark with the protocol of this chapter.
// A: look up keys in a std::map.  B: look up the same keys in a sorted std::vector with
// std::lower_bound. Same keys, same lookups, same result checked for both.
// Protocol: record the environment; 3 warm-up rounds of each; 21 timed rounds in which the
// order alternates (A then B, then B then A, ...) so slow drift hits both variants equally;
// report medians, the ratio and its 95 % bootstrap interval (abstat.hpp). Timing build: -O2.
#include "abstat.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

using Clock = std::chrono::steady_clock;

std::uint64_t mix(std::uint64_t x)
{
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

int main()
{
    std::size_t const n_keys = 200000;
    std::size_t const n_lookups = 1000000;
    std::map<std::uint64_t, std::uint64_t> tree;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> sorted;
    for (std::uint64_t i = 0; i < n_keys; ++i) {
        tree.emplace(mix(i), i);
        sorted.emplace_back(mix(i), i);
    }
    std::sort(sorted.begin(), sorted.end());
    std::vector<std::uint64_t> probes(n_lookups);
    for (std::size_t i = 0; i < n_lookups; ++i) {
        probes[i] = mix(mix(i) % n_keys);  // every probe hits an existing key
    }

    auto run_a = [&] {
        std::uint64_t sum = 0;
        for (std::uint64_t k : probes) {
            sum += tree.find(k)->second;
        }
        return sum;
    };
    auto run_b = [&] {
        std::uint64_t sum = 0;
        for (std::uint64_t k : probes) {
            auto it =
                std::lower_bound(sorted.begin(), sorted.end(), std::make_pair(k, std::uint64_t{0}));
            sum += it->second;
        }
        return sum;
    };
    auto time_ms = [](auto&& work, std::uint64_t& out) {
        auto const t0 = Clock::now();
        out = work();
        return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
    };

    std::string cpu = "unknown";
    std::ifstream info("/proc/cpuinfo");
    for (std::string line; std::getline(info, line);) {
        if (line.rfind("model name", 0) == 0) {
            cpu = line.substr(line.find(':') + 2);
            break;
        }
    }
    std::printf("environment: %s; build -O2; keys %zu; lookups per round %zu\n", cpu.c_str(),
                n_keys, n_lookups);

    std::uint64_t ra = 0;
    std::uint64_t rb = 0;
    for (int w = 0; w < 3; ++w) {
        time_ms(run_a, ra);
        time_ms(run_b, rb);
    }
    std::vector<double> a;
    std::vector<double> b;
    for (int r = 0; r < 21; ++r) {
        if (r % 2 == 0) {
            a.push_back(time_ms(run_a, ra));
            b.push_back(time_ms(run_b, rb));
        } else {
            b.push_back(time_ms(run_b, rb));
            a.push_back(time_ms(run_a, ra));
        }
    }
    std::printf("same answer from both variants: %s\n", ra == rb ? "yes" : "NO");
    auto show = [](char const* name, std::vector<double> const& v) {
        std::printf("%s rounds (ms, in run order):", name);
        for (double x : v) {
            std::printf(" %.1f", x);
        }
        std::printf("\n");
    };
    show("A std::map   ", a);
    show("B sorted vec ", b);
    abstat::Interval const ci = abstat::ratio_ci(a, b);
    std::printf("median A %.2f ms, median B %.2f ms\n", abstat::median(a), abstat::median(b));
    std::printf("ratio B/A %.3f, 95 %% bootstrap interval [%.3f, %.3f]\n", ci.ratio, ci.low,
                ci.high);
    return ra == rb ? 0 : 1;
}

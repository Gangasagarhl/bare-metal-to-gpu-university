// timing30.cc - MA202 course lab: time a program 30 times, report the median and spread.
// Built WITHOUT sanitizers and with -O2 by run.sh (sanitizers change what is measured).
#include "benchstats.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

int main()
{
    // The work being timed: sort a copy of 200,000 pseudo-random integers.
    std::mt19937 engine(63);
    std::vector<std::uint32_t> data(200000);
    for (auto& v : data) {
        v = engine();
    }
    auto work = [&data] {
        std::vector<std::uint32_t> copy = data;
        std::sort(copy.begin(), copy.end());
        benchstats::keep(copy[copy.size() / 2]);
    };
    // One cold run first, timed on its own, to show why warm-up runs exist.
    auto const start = std::chrono::steady_clock::now();
    work();
    auto const stop = std::chrono::steady_clock::now();
    double const cold = std::chrono::duration<double, std::milli>(stop - start).count();
    // The protocol: 3 warm-up runs, then 30 timed runs.
    std::vector<double> ms = benchstats::timeRuns(work, 3, 30);
    std::cout << "cold first run: " << cold << " ms (not part of the 30)\n";
    std::cout << "30 timed runs in the order measured (ms):\n";
    for (std::size_t i = 0; i < ms.size(); ++i) {
        std::cout << ms[i] << (i % 6 == 5 ? "\n" : "  ");
    }
    benchstats::Summary s = benchstats::summarise(ms);
    benchstats::print(std::cout, s, "ms");
    std::cout << "report: median " << s.median << " ms, IQR " << s.iqr << " ms (n = " << s.n
              << ", 3 warm-ups)\n";
    return 0;
}

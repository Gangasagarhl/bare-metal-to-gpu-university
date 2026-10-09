// capture.cc - evidence capture for the F0-63 forensic lab "The benchmark that lied".
// Version A sums the readings with a loop, version B with std::accumulate.
// Both call readings(), which builds the data on its FIRST call - inside a timed run.
#include "benchstats.hpp"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

std::vector<std::int32_t> const& readings()
{
    static std::vector<std::int32_t> const data = [] {
        std::vector<std::int32_t> v(4000000);
        for (std::size_t i = 0; i < v.size(); ++i) {
            v[i] = static_cast<std::int32_t>((i * 7919u) % 1000u);
        }
        return v;
    }();
    return data;
}

std::int64_t versionA()
{
    std::int64_t sum = 0;
    for (std::int32_t x : readings()) {
        sum += x;
    }
    return sum;
}

std::int64_t versionB()
{
    auto const& r = readings();
    return std::accumulate(r.begin(), r.end(), std::int64_t{0});
}

template <typename F>
double timeOnce(F f)
{
    auto const start = std::chrono::steady_clock::now();
    benchstats::keep(f());
    auto const stop = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

int main()
{
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "# run  A_ms  B_ms   (A and B alternate: A1, B1, A2, B2, ...; no warm-up)\n";
    for (int run = 1; run <= 30; ++run) {
        double const a = timeOnce(versionA);
        double const b = timeOnce(versionB);
        std::cout << run << " " << a << " " << b << "\n";
    }
    return 0;
}

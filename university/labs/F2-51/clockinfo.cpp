// clockinfo.cpp - what the C++ clocks promise, and the smallest step we can observe.
#include <chrono>
#include <cstdio>

template <typename Clock>
void describe(char const* name)
{
    using period = typename Clock::period;
    // Smallest non-zero difference between two consecutive readings, over many tries.
    auto best = Clock::duration::max();
    for (int i = 0; i < 100000; ++i) {
        auto const a = Clock::now();
        auto b = Clock::now();
        while (b == a) {
            b = Clock::now();
        }
        if (b - a < best) {
            best = b - a;
        }
    }
    double const ns = std::chrono::duration<double, std::nano>(best).count();
    std::printf("%-22s is_steady=%d  tick=%lld/%lld s  smallest observed step=%.0f ns\n",
                name, Clock::is_steady ? 1 : 0,
                static_cast<long long>(period::num), static_cast<long long>(period::den), ns);
}

int main()
{
    describe<std::chrono::steady_clock>("steady_clock");
    describe<std::chrono::system_clock>("system_clock");
    describe<std::chrono::high_resolution_clock>("high_resolution_clock");
    return 0;
}

// work_host.cc - the same two loops as guest_cpuid.cc (200 million integer LCG steps, then
// 50 million double-precision steps), run directly on this machine and timed with
// std::chrono, for comparison with the guest's time stamps.
// Built by run.sh with -O2 like the guest program (not by run_lab.sh's sanitizer build).
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>

int main()
{
    const auto t0 = std::chrono::steady_clock::now();
    std::uint64_t x = 1;
    for (std::uint64_t i = 0; i < 200000000; ++i) {
        x = x * 6364136223846793005ull + 1442695040888963407ull;
        __asm__ volatile("" : "+r"(x));
    }
    const auto t1 = std::chrono::steady_clock::now();
    double y = 1.0;
    for (int i = 0; i < 50000000; ++i) {
        y = y * 0.999999 + 0.5;
        __asm__ volatile("" : "+x"(y));
    }
    const auto t2 = std::chrono::steady_clock::now();
    std::uint64_t b = 0;
    std::memcpy(&b, &y, sizeof b);
    std::printf("host: integer work result 0x%016llx, took %.3f s\n",
                static_cast<unsigned long long>(x), std::chrono::duration<double>(t1 - t0).count());
    std::printf("host: floating-point work result bits 0x%016llx, took %.3f s\n",
                static_cast<unsigned long long>(b), std::chrono::duration<double>(t2 - t1).count());
    std::printf("(times measured in this shared container, not properties of any processor)\n");
    return 0;
}

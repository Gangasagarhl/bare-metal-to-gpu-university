// F6-32 Listing 2: NVTX ranges in a host-only program. With no tool attached the
// calls do nothing visible, so the program runs anywhere, including this build.
// Under a timeline profiler, the ranges appear as named bars on this thread.
#include <cstdio>
#include <vector>
#include <nvtx3/nvToolsExt.h>

int main()
{
    nvtxRangePushA("whole program");
    nvtxRangePushA("fill");
    std::vector<double> v(1 << 20);
    for (std::size_t i = 0; i < v.size(); ++i) {
        v[i] = static_cast<double>(i % 10);
    }
    nvtxRangePop();
    nvtxRangePushA("sum");
    double sum = 0.0;
    for (double x : v) {
        sum += x;
    }
    const int popResult = nvtxRangePop();                 // level ended, or negative
    nvtxMarkA("sum done");
    nvtxRangePop();
    const long long n = 1 << 20;
    const long long r = n % 10;
    const long long expected = n / 10 * 45 + r * (r - 1) / 2;   // full cycles + remainder
    std::printf("sum = %.0f (expected %lld)\n", sum, expected);
    std::printf("nvtxRangePop returned %d (no tool was attached to this run)\n", popResult);
    return 0;
}

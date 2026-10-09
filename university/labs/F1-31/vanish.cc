// F1-31 forensic evidence: Ana's benchmark of a loop. run.sh builds it at -O0 and at -O2.
#include <chrono>
#include <cstdio>

int main()
{
    const auto t0 = std::chrono::steady_clock::now();
    long s = 0;
    for (long i = 0; i < 100000000; ++i) {
        s += i % 7;
    }
    (void)s;  // silence the "set but not used" warning
    const auto t1 = std::chrono::steady_clock::now();
    std::printf("loop took %.3f ms\n", std::chrono::duration<double, std::milli>(t1 - t0).count());
    return 0;
}

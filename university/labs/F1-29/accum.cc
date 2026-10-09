// F1-29 Listing 2 (forensic evidence): add up 2^20 doubles with one running sum and with
// four running sums, and time both on the machine it runs on. Built with -O2 by run.sh.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

__attribute__((noinline)) double sumOne(const std::vector<double>& v)
{
    double s = 0.0;
    for (std::size_t i = 0; i < v.size(); ++i) {
        s += v[i];
    }
    return s;
}

__attribute__((noinline)) double sumFour(const std::vector<double>& v)
{
    double s0 = 0.0, s1 = 0.0, s2 = 0.0, s3 = 0.0;
    std::size_t i = 0;
    for (; i + 4 <= v.size(); i += 4) {
        s0 += v[i];
        s1 += v[i + 1];
        s2 += v[i + 2];
        s3 += v[i + 3];
    }
    for (; i < v.size(); ++i) {
        s0 += v[i];
    }
    return (s0 + s1) + (s2 + s3);
}

template <typename F>
static double medianMs(F f, const std::vector<double>& v, double& result)
{
    std::vector<double> ms;
    for (int rep = 0; rep < 21; ++rep) {
        const auto t0 = std::chrono::steady_clock::now();
        result = f(v);
        const auto t1 = std::chrono::steady_clock::now();
        ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    std::sort(ms.begin(), ms.end());
    return ms[ms.size() / 2];
}

int main()
{
    std::vector<double> v(1 << 20);
    for (std::size_t i = 0; i < v.size(); ++i) {
        v[i] = static_cast<double>(i % 1000) * 0.5;  // halves: every partial sum is exact
    }
    double r1 = 0.0, r4 = 0.0;
    const double t1 = medianMs(sumOne, v, r1);
    const double t4 = medianMs(sumFour, v, r4);
    std::printf("one running sum : result %.1f, median of 21 runs %.3f ms\n", r1, t1);
    std::printf("four running sums: result %.1f, median of 21 runs %.3f ms\n", r4, t4);
    std::printf("ratio one/four: %.2f\n", t1 / t4);
    return r1 == r4 ? 0 : 1;
}

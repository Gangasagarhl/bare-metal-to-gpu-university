// F0-80 Listing 4 (course lab): set the tolerance for a reduction kernel test, and prove the test
// can fail. The kernel is represented by its CPU order model; two planted bugs are the negative
// controls.
#include <cmath>
#include <cstdio>
#include <vector>

#include "reduce_model.hpp"

std::vector<float> uniformData(std::size_t n, std::uint64_t seed)
{
    std::vector<float> x(n);
    for (auto& v : x) {
        v = std::ldexp(static_cast<float>(splitmix64(seed) >> 40), -24); // [0, 1), exact in float
    }
    return x;
}

std::vector<float> mixedData(std::size_t n, std::uint64_t seed)
{
    std::vector<float> x(n);
    for (auto& v : x) {
        v = static_cast<float>(uniformMinus1To1(seed) *
                               std::pow(10.0, static_cast<int>(splitmix64(seed) % 7) - 3));
    }
    return x;
}

// Test 1: small integers. Every partial sum is an integer below 2^24, so float addition is exact
// in ANY order: the result must equal the integer sum exactly. Catches indexing bugs of any size.
bool exactIntegerTest(std::size_t n, Variant v)
{
    std::vector<float> x(n);
    std::uint64_t want = 0;
    for (std::size_t i = 0; i < n; ++i) {
        x[i] = static_cast<float>(i % 7);
        want += i % 7;
    }
    return static_cast<double>(sumKernelModel(x, v)) == static_cast<double>(want);
}

// Test 2: random data against the Kahan reference with the derived tolerance.
bool toleranceTest(const std::vector<float>& x, Variant v, double* errOverTol)
{
    const double err = std::fabs(static_cast<double>(sumKernelModel(x, v)) - referenceSum(x));
    const double tol = reductionTolerance(x);
    *errOverTol = err / tol;
    return err <= tol;
}

int main()
{
    const std::size_t sizes[] = {1, 255, 256, 257, 65536, 1000000, (std::size_t{1} << 20) + 3};
    const char* names[] = {"correct kernel", "bug: drops tail", "bug: drops last"};
    for (int vi = 0; vi < 3; ++vi) {
        const Variant v = static_cast<Variant>(vi);
        std::printf("%s\n  %-9s %-8s %-22s %-22s\n", names[vi], "n", "exact", "uniform (err/tol)",
                    "mixed (err/tol)");
        for (std::size_t n : sizes) {
            double r1 = 0.0, r2 = 0.0;
            const bool e = exactIntegerTest(n, v);
            const bool t1 = toleranceTest(uniformData(n, n + 1), v, &r1);
            const bool t2 = toleranceTest(mixedData(n, n + 2), v, &r2);
            std::printf("  %-9zu %-8s %-4s (%-9.3g)        %-4s (%-9.3g)\n", n, e ? "PASS" : "FAIL",
                        t1 ? "PASS" : "FAIL", r1, t2 ? "PASS" : "FAIL", r2);
        }
    }
    return 0;
}

// F0-75 Listing 1 (course lab): sum 2^20 floats in different orders and compare with a reference.
#include <cmath>
#include <cstdio>
#include <vector>

#include "data.hpp"

void compare(const char* title, std::vector<float> x)
{
    const double ref = referenceSum(x);
    const double absSum = sumAbs(x);
    const double u = std::ldexp(1.0, -24);
    const double n = static_cast<double>(x.size());
    std::printf("%s: n = %zu, reference (Kahan in double) = %.10g, sum|x| = %.10g\n", title,
                x.size(), ref, absSum);
    std::printf("  %-26s %-16s %-12s %-10s\n", "order", "float result", "rel. error",
                "error/bound");
    auto row = [&](const char* name, float s, double depth) {
        const double err = std::fabs(static_cast<double>(s) - ref);
        const double bound = gamma(depth, u) * absSum;
        std::printf("  %-26s %-16.9g %-12.3g %-10.3g\n", name, static_cast<double>(s),
                    err / std::fabs(ref), err / bound);
    };
    row("forward", sumForward(x), n - 1);
    row("backward", sumBackward(x), n - 1);
    std::vector<float> sorted = x;
    std::sort(sorted.begin(), sorted.end(),
              [](float a, float b) { return std::fabs(a) < std::fabs(b); });
    row("increasing |x|", sumForward(sorted), n - 1);
    std::sort(sorted.begin(), sorted.end(),
              [](float a, float b) { return std::fabs(a) > std::fabs(b); });
    row("decreasing |x|", sumForward(sorted), n - 1);
    row("pairwise", sumPairwise(x.data(), x.size()), std::ceil(std::log2(n)));
    row("GPU model, 128 blocks", sumGpuModel(x, 128), n / (128.0 * 256.0) - 1 + 8 + 127);
    row("GPU model, 192 blocks", sumGpuModel(x, 192), std::ceil(n / (192.0 * 256.0)) - 1 + 8 + 191);
    row("Kahan (float)", sumKahan(x), 2.0); // bound used: about 2u sum|x| (B1)
    std::printf("\n");
}

int main()
{
    const std::size_t n = std::size_t{1} << 20;
    compare("uniform [0,1)", uniformData(n, 2026));
    compare("mixed signs, 1e-3..1e3", mixedData(n, 2026));
    return 0;
}

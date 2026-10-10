// F0-80 worked example check: the tolerance for one run of reduce.cu (n = 2^20 + 3, data in [0,
// 1)).
#include <cmath>
#include <cstdio>
#include <vector>

#include "reduce_model.hpp"

int main()
{
    const std::size_t n = (std::size_t{1} << 20) + 3;
    std::vector<float> x(n);
    std::uint64_t seed = n + 1;
    double absSum = 0.0;
    for (auto& v : x) {
        v = std::ldexp(static_cast<float>(splitmix64(seed) >> 40), -24);
        absSum += static_cast<double>(v);
    }
    const double perThread = std::ceil(static_cast<double>(n) / (kBlocks * kThreads));
    const double depth = kernelDepth(n);
    const double u = std::ldexp(1.0, -24);
    std::printf("elements per thread (max) = %.0f; depth = %.0f - 1 + 8 + %zu = %.0f\n", perThread,
                perThread, kBlocks - 1, depth);
    std::printf("gamma_depth = %.6g   (depth * u = %.6g)\n", depth * u / (1.0 - depth * u),
                depth * u);
    std::printf("sum|x| = %.10g; tolerance = %.6g (relative to the sum: %.3g)\n", absSum,
                reductionTolerance(x), reductionTolerance(x) / referenceSum(x));
    const double got = static_cast<double>(sumKernelModel(x));
    std::printf(
        "model result = %.9g; reference = %.12g; |error| = %.4g; error / tolerance = %.3g\n", got,
        referenceSum(x), std::fabs(got - referenceSum(x)),
        std::fabs(got - referenceSum(x)) / reductionTolerance(x));
    return 0;
}

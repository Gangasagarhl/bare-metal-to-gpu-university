// F0-75 forensic answer-key run: who is right, and a test that is right for a reason.
#include <cmath>
#include <cstdio>
#include <vector>

#include "data.hpp"

int main()
{
    const double u = std::ldexp(1.0, -24);
    const std::size_t sizes[] = {std::size_t{1} << 20, std::size_t{1} << 22};
    const std::uint64_t seeds[] = {101, 102};
    std::printf("%-9s %-14s %-12s %-12s %-12s %s\n", "n", "reference", "err gpu", "err cpu",
                "tolerance", "gpu within tolerance");
    for (int k = 0; k < 2; ++k) {
        const std::vector<float> x = uniformData(sizes[k], seeds[k]);
        const double ref = referenceSum(x);
        const double gpu = static_cast<double>(sumGpuModel(x, 128));
        const double cpu = static_cast<double>(sumForward(x));
        // Depth of the kernel's addition tree: grid-stride chain + 8 tree levels + 127 block
        // additions.
        const double n = static_cast<double>(x.size());
        const double depth = std::ceil(n / (128.0 * 256.0)) - 1 + 8 + 127;
        const double tol = gamma(depth, u) * sumAbs(x);
        std::printf("%-9zu %-14.10g %-12.4g %-12.4g %-12.4g %s\n", x.size(), ref,
                    std::fabs(gpu - ref), std::fabs(cpu - ref), tol,
                    std::fabs(gpu - ref) <= tol ? "yes" : "NO");
    }
    return 0;
}

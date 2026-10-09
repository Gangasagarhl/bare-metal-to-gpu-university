// F0-80 forensic evidence: the CI log of test_sum_kernel, and the field report that started the
// hunt. The kernel is represented by its CPU order model (reduce_model.hpp). Two things are
// deliberately wrong: one in the kernel and one in the test (see the answer key).
#include <cmath>
#include <cstdio>
#include <vector>

#include "reduce_model.hpp"

int main()
{
    std::printf("test_sum_kernel: |gpu - cpu| <= 1e-2 * |cpu|  (cpu = plain float loop; tolerance "
                "widened in\n"
                "                 commit 'stop flaky failures')\n");
    std::printf("%-9s %-16s %-16s %-11s %s\n", "n", "gpu", "cpu", "rel. diff", "verdict");
    const std::size_t sizes[] = {1024, 4096, 65536, 1000000, 1048576, 4194300};
    for (std::size_t n : sizes) {
        std::uint64_t seed = n;
        std::vector<float> x(n);
        for (auto& v : x) {
            v = std::ldexp(static_cast<float>(splitmix64(seed) >> 40), -24);
        }
        const float gpu = sumKernelModel(x, Variant::DropTail);
        float cpu = 0.0f;
        for (float v : x) {
            cpu += v;
        }
        const double rel =
            std::fabs(static_cast<double>(gpu) - cpu) / std::fabs(static_cast<double>(cpu));
        std::printf("%-9zu %-16.9g %-16.9g %-11.3g %s\n", n, static_cast<double>(gpu),
                    static_cast<double>(cpu), rel, rel <= 1e-2 ? "PASS" : "FAIL");
    }
    std::printf(
        "\nfield report: frame of 1000000 pixels, all counted as 1.0f; total reported by the GPU = "
        "%.1f\n",
        static_cast<double>(sumKernelModel(std::vector<float>(1000000, 1.0f), Variant::DropTail)));
    return 0;
}

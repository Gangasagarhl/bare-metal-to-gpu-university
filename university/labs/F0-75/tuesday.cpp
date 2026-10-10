// F0-75 forensic evidence: the CI log of "test_reduce_matches_cpu".
// The "GPU result" comes from the university's CPU model of the kernel's addition order (data.hpp).
// Deliberate fault (see the answer key): the test's reference and tolerance, not the kernel.
#include <cmath>
#include <cstdio>
#include <vector>

#include "data.hpp"

int main()
{
    struct Night
    {
        const char* day;
        std::size_t n;
        std::size_t blocks;
        std::uint64_t seed;
    };
    const Night nights[] = {
        {"Mon", std::size_t{1} << 20, 128, 101}, {"Tue", std::size_t{1} << 22, 128, 102},
        {"Wed", std::size_t{1} << 20, 128, 103}, {"Thu", std::size_t{1} << 20, 128, 104},
        {"Fri", std::size_t{1} << 20, 128, 105}, {"Mon", std::size_t{1} << 20, 128, 106},
        {"Tue", std::size_t{1} << 22, 128, 107}, {"Wed", std::size_t{1} << 20, 128, 108},
    };
    std::printf("test_reduce_matches_cpu: EXPECT_NEAR(gpu, cpu, 16.0f), cpu = plain float loop\n");
    std::printf("%-4s %-9s %-16s %-16s %-10s %s\n", "day", "n", "gpu", "cpu", "|diff|", "verdict");
    for (const Night& night : nights) {
        const std::vector<float> x = uniformData(night.n, night.seed);
        const float gpu = sumGpuModel(x, night.blocks);
        const float cpu = sumForward(x);
        const float diff = std::fabs(gpu - cpu);
        std::printf("%-4s %-9zu %-16.9g %-16.9g %-10.6g %s\n", night.day, night.n,
                    static_cast<double>(gpu), static_cast<double>(cpu), static_cast<double>(diff),
                    diff <= 16.0f ? "PASS" : "FAIL");
    }
    return 0;
}

// F6-25 forensic evidence: two test reports for the same two BF16-input, FP32-accumulate
// dot-product "kernels" (modelled on the CPU with lowp.hpp). Kernel "good" is correct;
// kernel "drops-last" forgets the last term of every dot product. Each is checked on two
// data sets with (a) the team's fixed tolerance |c - ref| <= 0.01 and (b) a per-element
// bound from the input format and K. ref = FP64 dot product of the original FP32 values.
// Data set "integers" holds whole numbers in [-4, 4]: exact in BF16, and every partial sum
// (at most 16 * 1024 in size) is exact in FP32, so (c) can demand equality.
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>
#include "lowp.hpp"

int main()
{
    const int K = 1024, rows = 64;
    const double uin = unitRoundoff(BF16), uacc = unitRoundoff(FP32);
    const double g = K * uacc / (1 - K * uacc);
    const double factor = (2 * uin + uin * uin) * (1 + g) + g;
    std::printf("K = %d, %d dot products per data set; bound factor = %.6g (times sum|a*b|)\n",
                K, rows, factor);
    std::printf("%-11s %-9s | %-28s | %-28s | %s\n", "kernel", "data", "(a) fixed |err| <= 0.01",
                "(b) bound factor*sum|a*b|", "(c) exact");
    for (const char* data : {"small", "large", "integers"}) {
        const float scale = data[0] == 's' ? 0.05f : 4.0f;          // values in [-scale, scale]
        const bool ints = data[0] == 'i';
        for (bool dropLast : {false, true}) {
            std::mt19937 gen(5u);
            std::uniform_real_distribution<float> dist(-scale, scale);
            int failA = 0, failB = 0, failC = 0;
            double worstA = 0.0, worstB = 0.0;
            for (int r = 0; r < rows; ++r) {
                double exact = 0.0, sumAbs = 0.0, c = 0.0;
                for (int k = 0; k < K; ++k) {
                    float a = dist(gen), b = dist(gen);
                    if (ints) {
                        a = std::round(a);
                        b = std::round(b);
                    }
                    exact += double(a) * b;
                    sumAbs += std::fabs(double(a) * b);
                    if (!(dropLast && k == K - 1)) {
                        c = roundTo(c + roundTo(a, BF16) * roundTo(b, BF16), FP32);
                    }
                }
                const double err = std::fabs(c - exact);
                failA += err > 0.01 ? 1 : 0;
                failB += err > factor * sumAbs ? 1 : 0;
                failC += c != exact ? 1 : 0;
                worstA = std::fmax(worstA, err / 0.01);
                worstB = std::fmax(worstB, err / (factor * sumAbs));
            }
            std::printf("%-11s %-9s | %2d of %d fail, worst %8.3fx | %2d of %d fail, worst %6.3fx | ",
                        dropLast ? "drops-last" : "good", data, failA, rows, worstA, failB, rows,
                        worstB);
            if (ints) {
                std::printf("%2d of %d fail\n", failC, rows);
            } else {
                std::printf("not applicable\n");
            }
        }
    }
    return 0;
}

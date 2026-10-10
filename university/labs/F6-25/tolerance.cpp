// F6-25 Listing 2: how large is the error of a mixed-precision dot product, and which
// tolerance does a test need? Inputs are FP32 values in [-1, 1] (or [0, 1]) rounded to
// the input format; products are exact; sums are rounded to the accumulator format after
// every addition, in order. Error is measured against the exact (FP64) dot product of the
// ORIGINAL FP32 inputs and divided by sum |a*b|, so the bound has no unknowns:
//   bound = (2 u_in + u_in^2)(1 + gamma_K) + gamma_K,   gamma_K = K u_acc / (1 - K u_acc).
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>
#include "lowp.hpp"

double gammaK(int K, const Format& acc)
{
    const double u = unitRoundoff(acc);
    return K * u / (1.0 - K * u);
}

int main()
{
    const Format* inputs[] = {&FP32, &TF32, &BF16, &FP16, &E4M3};
    std::printf("%-9s %-5s %-6s %-9s | %-11s %-11s %-6s\n", "input", "acc", "K", "inputs",
                "max error", "bound", "ratio");
    for (bool positive : {false, true}) {
        for (const Format* inp : inputs) {   // FP32 row: plain FP32 SGEMM, for comparison
            const Format& in = *inp;
            for (const Format* accp : {&FP32, &FP16}) {
                const Format& acc = *accp;
                if (&acc == &FP16 && &in != &FP16) {
                    continue;                      // FP16 accumulation shown for FP16 inputs only
                }
                for (int K : {16, 256, 4096}) {
                    std::mt19937 gen(11u);
                    std::uniform_real_distribution<float> dist(positive ? 0.0f : -1.0f, 1.0f);
                    double worst = 0.0;
                    for (int trial = 0; trial < 200; ++trial) {
                        double exact = 0.0, sumAbs = 0.0, c = 0.0;
                        for (int k = 0; k < K; ++k) {
                            const float a = dist(gen), b = dist(gen);
                            exact += double(a) * double(b);
                            sumAbs += std::fabs(double(a) * double(b));
                            const double p = roundTo(a, in) * roundTo(b, in);   // exact product
                            c = roundTo(c + p, acc);
                        }
                        worst = std::fmax(worst, std::fabs(c - exact) / sumAbs);
                    }
                    const double uin = in.p == FP32.p ? 0.0 : unitRoundoff(in);
                    const double g = gammaK(K, acc);
                    const double bound = (2 * uin + uin * uin) * (1 + g) + g;
                    if (K * unitRoundoff(acc) >= 1.0) {   // the bound needs K u < 1
                        std::printf("%-9s %-5s %-6d %-9s | %-11.4g %-11s %-6s\n", in.name, acc.name,
                                    K, positive ? "[0,1]" : "[-1,1]", worst, "none: K*u>=1", "-");
                        continue;
                    }
                    std::printf("%-9s %-5s %-6d %-9s | %-11.4g %-11.4g %-6.3f\n", in.name, acc.name,
                                K, positive ? "[0,1]" : "[-1,1]", worst, bound, worst / bound);
                }
            }
        }
    }
    return 0;
}

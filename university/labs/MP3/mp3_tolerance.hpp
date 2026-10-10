// MP3 Listing 1: the tolerance policy of the MP3 correctness suite, one place for all tests.
// A GEMM element is a K-term dot product. Its error against the exact (FP64) dot product of
// the ORIGINAL FP32 inputs, divided by sum |a*b|, is bounded by (F6-25 Listing 2, MA302):
//   bound(K) = (2 u_in + u_in^2)(1 + gamma_K) + gamma_K,   gamma_K = K u_acc / (1 - K u_acc)
// u_in  = unit roundoff of the input format (0 when inputs stay FP32: nothing is rounded),
// u_acc = unit roundoff of the accumulator (FP32 for every MP3 kernel: 2^-24).
// The bound holds for ANY summation order, so it is valid for every tiling the library uses.
#pragma once
#include <cmath>
#include "../F6-25/lowp.hpp"

struct Precision
{
    const char* name;      // name used in test reports and in the raw benchmark records
    Format input;          // format the inputs are rounded to before the multiply
    bool roundsInputs;     // false for FP32 SGEMM: the inputs are used as given
};

inline constexpr Precision P_FP32{"fp32", FP32, false};
inline constexpr Precision P_FP16{"fp16-in/fp32-acc", FP16, true};
inline constexpr Precision P_BF16{"bf16-in/fp32-acc", BF16, true};

inline double gammaFp32(int K)
{
    const double u = unitRoundoff(FP32);
    return K * u / (1.0 - K * u);              // needs K u < 1: true for K < 2^24
}

// Relative bound: |computed - exact| <= relBound(p, K) * sum |a*b|.
inline double relBound(const Precision& p, int K)
{
    const double uin = p.roundsInputs ? unitRoundoff(p.input) : 0.0;
    const double g = gammaFp32(K);
    return (2.0 * uin + uin * uin) * (1.0 + g) + g;
}

// One element: true when inside the bound. A NaN (an element never written) fails.
inline bool withinTolerance(double got, double exact, double sumAbs, const Precision& p, int K)
{
    const double err = std::fabs(got - exact);
    return err <= relBound(p, K) * sumAbs + 1e-30;   // + tiny: an all-zero row has bound 0
}

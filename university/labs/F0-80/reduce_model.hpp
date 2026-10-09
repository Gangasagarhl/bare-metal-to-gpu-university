// F0-80 shared code: the CPU model of reduce.cu's addition order, the E3 reference and the
// tolerance.
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "lowp.hpp"

inline constexpr std::size_t kThreads = 256; // threads per block, as in reduce.cu
inline constexpr std::size_t kBlocks = 128;  // blocks, as in reduce.cu

// tail: how many elements at the end the (possibly buggy) loop bound leaves out.
enum class Variant { Correct, DropTail, DropLast };

inline std::size_t loopEnd(std::size_t n, Variant v)
{
    if (v == Variant::DropTail) {
        return n / kThreads * kThreads; // bug: forgets the last n % 256 elements
    }
    if (v == Variant::DropLast) {
        return n == 0 ? 0 : n - 1; // bug: off by one
    }
    return n;
}

// Same order as reduce.cu: grid-stride loop per thread, tree per block, block partials added in
// order.
inline float sumKernelModel(const std::vector<float>& x, Variant v = Variant::Correct)
{
    const std::size_t end = loopEnd(x.size(), v);
    const std::size_t stride = kBlocks * kThreads;
    std::vector<float> part(kThreads);
    float total = 0.0f;
    for (std::size_t b = 0; b < kBlocks; ++b) {
        for (std::size_t t = 0; t < kThreads; ++t) {
            float s = 0.0f;
            for (std::size_t i = b * kThreads + t; i < end; i += stride) {
                s += x[i];
            }
            part[t] = s;
        }
        for (std::size_t half = kThreads / 2; half > 0; half /= 2) {
            for (std::size_t t = 0; t < half; ++t) {
                part[t] += part[t + half];
            }
        }
        total += part[0];
    }
    return total;
}

// E3 reference: Kahan summation in double.
inline double referenceSum(const std::vector<float>& x)
{
    double s = 0.0, c = 0.0;
    for (float v : x) {
        const double y = static_cast<double>(v) - c;
        const double t = s + y;
        c = (t - s) - y;
        s = t;
    }
    return s;
}

// Depth of the kernel's addition tree: the longest chain of roundings any input goes through.
inline double kernelDepth(std::size_t n)
{
    const double perThread =
        std::ceil(static_cast<double>(n) / static_cast<double>(kBlocks * kThreads));
    return (perThread > 0 ? perThread - 1 : 0) + 8 +
           static_cast<double>(kBlocks - 1); // 8 = log2(256)
}

// Tolerance: gamma_depth(u) * sum|x| (worst case for the kernel) + a margin for the reference
// itself.
inline double reductionTolerance(const std::vector<float>& x)
{
    const double u = std::ldexp(1.0, -24);
    double absSum = 0.0;
    for (float v : x) {
        absSum += std::fabs(static_cast<double>(v));
    }
    const double d = kernelDepth(x.size());
    const double uRef = std::ldexp(1.0, -53);
    return d * u / (1.0 - d * u) * absSum + 4.0 * uRef * absSum;
}

// F0-75 shared helpers: reproducible test data and the summation orders compared in this chapter.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

// splitmix64: a small, fully specified generator, so every run and every machine gets the same
// data.
inline std::uint64_t splitmix64(std::uint64_t& state)
{
    std::uint64_t z = (state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

// Uniform floats in [0, 1): 24 random bits times 2^-24, so each value is exact in float.
inline std::vector<float> uniformData(std::size_t n, std::uint64_t seed)
{
    std::vector<float> x(n);
    for (auto& v : x) {
        v = std::ldexp(static_cast<float>(splitmix64(seed) >> 40), -24);
    }
    return x;
}

// Mixed signs and magnitudes from 1e-3 to 1e3: a harder case with cancellation.
inline std::vector<float> mixedData(std::size_t n, std::uint64_t seed)
{
    std::vector<float> x(n);
    for (auto& v : x) {
        const double r = std::ldexp(static_cast<double>(splitmix64(seed) >> 11), -53); // [0, 1)
        const int decade = static_cast<int>(splitmix64(seed) % 7) - 3;                 // -3 .. 3
        v = static_cast<float>((2.0 * r - 1.0) * std::pow(10.0, decade));
    }
    return x;
}

inline float sumForward(const std::vector<float>& x)
{
    float s = 0.0f;
    for (float v : x) {
        s += v;
    }
    return s;
}

inline float sumBackward(const std::vector<float>& x)
{
    float s = 0.0f;
    for (std::size_t i = x.size(); i-- > 0;) {
        s += x[i];
    }
    return s;
}

// Pairwise: add the two halves' sums, recursively (a balanced tree of additions).
inline float sumPairwise(const float* x, std::size_t n)
{
    if (n == 1) {
        return x[0];
    }
    const std::size_t h = n / 2;
    return sumPairwise(x, h) + sumPairwise(x + h, n - h);
}

// Kahan (compensated) summation: c carries the low-order part lost by each addition.
inline float sumKahan(const std::vector<float>& x)
{
    float s = 0.0f;
    float c = 0.0f;
    for (float v : x) {
        const float y = v - c; // the new value, corrected by what was lost last time
        const float t = s + y; // big + small: the low bits of y are lost here ...
        c = (t - s) - y;       // ... and recovered here (exact when abs(y) <= abs(s))
        s = t;
    }
    return s;
}

// The E3 reference: Kahan summation carried out in double.
inline double referenceSum(const std::vector<float>& x)
{
    double s = 0.0;
    double c = 0.0;
    for (float v : x) {
        const double y = static_cast<double>(v) - c;
        const double t = s + y;
        c = (t - s) - y;
        s = t;
    }
    return s;
}

// The order of a GPU-style reduction (modelled on the CPU): `blocks` blocks of 256 threads;
// each thread first adds its elements with a grid-stride loop, each block then adds its 256
// partial sums as a tree (stride 128, 64, ..., 1), and the block results are added in order.
inline float sumGpuModel(const std::vector<float>& x, std::size_t blocks)
{
    const std::size_t threads = 256;
    const std::size_t stride = blocks * threads;
    float total = 0.0f;
    std::vector<float> part(threads);
    for (std::size_t b = 0; b < blocks; ++b) {
        for (std::size_t t = 0; t < threads; ++t) {
            float s = 0.0f;
            for (std::size_t i = b * threads + t; i < x.size(); i += stride) {
                s += x[i];
            }
            part[t] = s;
        }
        for (std::size_t half = threads / 2; half > 0; half /= 2) {
            for (std::size_t t = 0; t < half; ++t) {
                part[t] += part[t + half];
            }
        }
        total += part[0];
    }
    return total;
}

inline double sumAbs(const std::vector<float>& x)
{
    double s = 0.0;
    for (float v : x) {
        s += std::fabs(static_cast<double>(v));
    }
    return s;
}

// gamma_k = k u / (1 - k u): the bound factor for a chain of k roundings.
inline double gamma(double k, double u)
{
    return k * u / (1.0 - k * u);
}

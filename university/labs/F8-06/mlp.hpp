// F8-06 mlp.hpp: a very small model for data-parallel training on the CPU.
// y = w2 . tanh(W1 x + b1) + b2, with 4 inputs, 16 hidden units and 1 output (97 parameters).
// Loss: mean squared error over the global batch. The synthetic data depend only on the
// sample's index, so every worker count sees exactly the same samples.
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

constexpr std::size_t IN = 4, HID = 16;
constexpr std::size_t NPARAM = HID * IN + HID + HID + 1;   // W1, b1, w2, b2

struct Sample
{
    float x[IN];
    float t;
};

inline float uniform(std::uint32_t& s)        // fixed pseudo-random sequence in [-1, 1)
{
    s = s * 1664525u + 1013904223u;
    return static_cast<float>(s >> 8) / 8388608.0f - 1.0f;
}

inline Sample sampleAt(std::uint64_t index)   // sample number `index` of the data stream
{
    std::uint32_t s = static_cast<std::uint32_t>(index * 2654435761u + 7u);
    Sample a{};
    for (float& v : a.x) {
        v = uniform(s);
    }
    a.t = std::sin(2.0f * a.x[0]) + 0.5f * a.x[1] * a.x[2] - 0.3f * a.x[3];
    return a;
}

inline std::vector<float> initParams(std::uint32_t seed)
{
    std::vector<float> p(NPARAM);
    for (float& v : p) {
        v = 0.5f * uniform(seed);
    }
    return p;
}

// adds d(loss sum)/d(params) of samples [first, last) into grad; returns their loss sum
inline float lossAndGrad(const std::vector<float>& p, std::uint64_t first, std::uint64_t last,
                         std::vector<float>& grad)
{
    const float* W1 = p.data();
    const float* b1 = W1 + HID * IN;
    const float* w2 = b1 + HID;
    const float b2 = w2[HID];
    float* gW1 = grad.data();
    float* gb1 = gW1 + HID * IN;
    float* gw2 = gb1 + HID;
    float lossSum = 0.0f;
    for (std::uint64_t i = first; i < last; ++i) {
        const Sample a = sampleAt(i);
        float h[HID];
        float y = b2;
        for (std::size_t j = 0; j < HID; ++j) {
            float z = b1[j];
            for (std::size_t k = 0; k < IN; ++k) {
                z += W1[j * IN + k] * a.x[k];
            }
            h[j] = std::tanh(z);
            y += w2[j] * h[j];
        }
        const float e = y - a.t;
        lossSum += e * e;
        const float dy = 2.0f * e;                       // d(e^2)/dy
        for (std::size_t j = 0; j < HID; ++j) {
            gw2[j] += dy * h[j];
            const float dz = dy * w2[j] * (1.0f - h[j] * h[j]);
            gb1[j] += dz;
            for (std::size_t k = 0; k < IN; ++k) {
                gW1[j * IN + k] += dz * a.x[k];
            }
        }
        gw2[HID] += dy;                                   // b2
    }
    return lossSum;
}

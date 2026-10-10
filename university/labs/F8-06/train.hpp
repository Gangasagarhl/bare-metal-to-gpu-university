// F8-06 train.hpp: one data-parallel training run with N in-process workers.
// Every step: each worker computes the gradient of its shard of the global batch, one ring
// all-reduce (ring.hpp) adds the gradients and the loss sums of all workers, and every worker
// applies the same SGD update to its own copy (replica) of the parameters.
#pragma once
#include <cstdint>
#include <vector>
#include "mlp.hpp"
#include "ring.hpp"

enum class Mode
{
    Correct,          // same initial parameters on every worker; gradient = global mean
    NoBroadcast,      // BUG: every worker initialises its own parameters
    SumNotAverage     // BUG: each worker divides by its own shard size, then the ring adds
};

struct Run
{
    std::vector<float> loss;              // global mean loss before each step
    std::vector<float> replicaGap;        // largest parameter difference between replicas
    std::vector<float> params;            // worker 0's final parameters
};

inline Run train(std::size_t n, int steps, Mode mode)
{
    const std::uint64_t batch = 64;       // global batch, split evenly over the workers
    const float lr = 0.05f;
    std::vector<std::vector<float>> p(n);
    for (std::size_t w = 0; w < n; ++w) { // Correct: "broadcast" worker 0's parameters
        p[w] = initParams(mode == Mode::NoBroadcast ? 42u + static_cast<std::uint32_t>(w) : 42u);
    }
    Run run;
    for (int s = 0; s < steps; ++s) {
        std::vector<std::vector<float>> buf(n, std::vector<float>(NPARAM + 1, 0.0f));
        for (std::size_t w = 0; w < n; ++w) {
            const std::uint64_t first = static_cast<std::uint64_t>(s) * batch + w * batch / n;
            const std::uint64_t last = static_cast<std::uint64_t>(s) * batch + (w + 1) * batch / n;
            const float lossSum = lossAndGrad(p[w], first, last, buf[w]);
            const float scale = mode == Mode::SumNotAverage ? static_cast<float>(last - first)
                                                            : static_cast<float>(batch);
            for (std::size_t i = 0; i < NPARAM; ++i) {
                buf[w][i] /= scale;
            }
            buf[w][NPARAM] = lossSum;      // the loss travels in the same all-reduce
        }
        ringAllReduce(buf, 32);            // small chunks: the gradient is only 98 floats
        run.loss.push_back(buf[0][NPARAM] / static_cast<float>(batch));
        float gap = 0.0f;
        for (std::size_t w = 0; w < n; ++w) {
            for (std::size_t i = 0; i < NPARAM; ++i) {
                p[w][i] -= lr * buf[w][i];
                const float d = p[w][i] > p[0][i] ? p[w][i] - p[0][i] : p[0][i] - p[w][i];
                gap = d > gap ? d : gap;
            }
        }
        run.replicaGap.push_back(gap);
    }
    run.params = p[0];
    return run;
}

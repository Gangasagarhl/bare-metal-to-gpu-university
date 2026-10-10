// F8-06 Listing 1: data-parallel training of mlp.hpp on 1, 2, 4 and 8 in-process workers,
// with the ring all-reduce of F8-05. Milestone F3's acceptance test, on the CPU: with fixed
// seeds, N-worker training gives the same loss curve as 1-worker training within a stated
// tolerance (the global batch is the same, so the effective batch size does not change).
#include <cmath>
#include <cstdio>
#include "train.hpp"

int main()
{
    const int steps = 300;
    const float tolerance = 1e-4f;         // our stated tolerance on the loss
    const Run one = train(1, steps, Mode::Correct);
    std::printf("loss of the 1-worker run: step 0 %.5f, step 100 %.5f, step 200 %.5f, step 299 %.5f\n",
                static_cast<double>(one.loss[0]), static_cast<double>(one.loss[100]),
                static_cast<double>(one.loss[200]), static_cast<double>(one.loss[299]));
    bool pass = true;
    for (std::size_t n : {2u, 4u, 8u}) {
        const Run r = train(n, steps, Mode::Correct);
        float maxLoss = 0.0f, maxParam = 0.0f, maxGap = 0.0f;
        int stepsDiffer = 0;
        for (int s = 0; s < steps; ++s) {
            const float d = std::fabs(r.loss[static_cast<std::size_t>(s)] - one.loss[static_cast<std::size_t>(s)]);
            maxLoss = d > maxLoss ? d : maxLoss;
            stepsDiffer += d != 0.0f;
            maxGap = r.replicaGap[static_cast<std::size_t>(s)] > maxGap ? r.replicaGap[static_cast<std::size_t>(s)] : maxGap;
        }
        for (std::size_t i = 0; i < NPARAM; ++i) {
            const float d = std::fabs(r.params[i] - one.params[i]);
            maxParam = d > maxParam ? d : maxParam;
        }
        const bool ok = maxLoss <= tolerance && maxGap == 0.0f;
        pass = pass && ok;
        std::printf("N=%zu: loss differs from 1 worker at %d of %d steps, largest difference %.3g; "
                    "final parameters differ by up to %.3g; replicas identical: %s; %s\n",
                    n, stepsDiffer, steps, static_cast<double>(maxLoss), static_cast<double>(maxParam),
                    maxGap == 0.0f ? "yes" : "NO", ok ? "PASS" : "FAIL");
    }
    std::printf("acceptance (loss within %.0e of 1 worker, replicas identical): %s\n",
                static_cast<double>(tolerance), pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}

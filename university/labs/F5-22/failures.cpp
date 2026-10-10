// failures.cpp - why big jobs fail more often: every node a job uses must stay up
// for the whole run (F5-22, DS303). The failure probability is an exercise value,
// not a measurement of any real machine.
#include <cmath>
#include <cstdio>
#include <random>

constexpr double kFailPerNodeHour = 0.0005;  // assumed for the exercise
constexpr int kHours = 24;

// A uniform number in [0, 1) built from std::mt19937, whose output sequence the
// C++ standard fixes, so every compiler prints the same results for one seed.
double uniform01(std::mt19937& gen)
{
    return static_cast<double>(gen()) / 4294967296.0;
}

// Runs one attempt of a job on `nodes` nodes; returns the hour of the first
// failure (0..kHours-1), or -1 if the job finished. `failedNode` gets the node.
int attempt(std::mt19937& gen, int nodes, int& failedNode)
{
    for (int hour = 0; hour < kHours; ++hour) {
        for (int n = 0; n < nodes; ++n) {
            if (uniform01(gen) < kFailPerNodeHour) {
                failedNode = n;
                return hour;
            }
        }
    }
    return -1;
}

int main()
{
    std::mt19937 gen(2026);
    std::printf("assumed failure probability per node per hour: %.4f; job length %d h\n\n",
                kFailPerNodeHour, kHours);
    std::printf("%6s  %12s  %14s\n", "nodes", "P(success)", "simulated");
    for (int nodes : {1, 8, 64, 512}) {
        const double formula = std::pow(1.0 - kFailPerNodeHour, nodes * kHours);
        int ok = 0;
        const int trials = 2000;
        for (int t = 0; t < trials; ++t) {
            int failedNode = -1;
            if (attempt(gen, nodes, failedNode) < 0) {
                ++ok;
            }
        }
        std::printf("%6d  %12.4f  %8d/%d\n", nodes, formula, ok, trials);
    }

    std::printf("\nlog of 12 attempts of one 64-node job (nodes numbered 0..63):\n");
    for (int a = 1; a <= 12; ++a) {
        int failedNode = -1;
        const int hour = attempt(gen, 64, failedNode);
        if (hour < 0) {
            std::printf("attempt %2d: COMPLETED after %d h\n", a, kHours);
        } else {
            std::printf("attempt %2d: FAILED in hour %2d, node %2d stopped responding\n", a, hour,
                        failedNode);
        }
    }
    return 0;
}

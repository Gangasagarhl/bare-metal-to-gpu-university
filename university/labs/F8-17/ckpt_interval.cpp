// F8-17 Listing 1: how often should a long job checkpoint? A Monte Carlo model.
// A job needs W hours of computation. Every T hours of work it writes a checkpoint (C hours).
// Failures of the job arrive at random, independently, with mean time between failures M
// (exponential distribution); a job on n nodes that each fail with mean M_node has M = M_node / n.
// After a failure the job loses the work since its last checkpoint and pays R hours to restart.
// The program compares intervals T with the first-order optimum T* = sqrt(2 C M) (derived in the
// chapter). Input lines: label W C R M_node nodes. All values are invented teaching values.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <random>
#include <string>
#include <vector>

double exponential(std::mt19937_64& rng, double mean)
{
    const double u = static_cast<double>(rng() >> 11) * 0x1.0p-53;   // uniform in [0, 1)
    return -mean * std::log(1.0 - u);
}

// wall-clock hours to finish W hours of work, one simulated run
double run(double W, double T, double C, double R, double M, std::mt19937_64& rng)
{
    double done = 0, wall = 0;
    while (done < W) {
        const double seg = std::fmin(T, W - done);
        const double need = seg + (done + seg < W ? C : 0.0);   // no checkpoint after the last piece
        const double fail = exponential(rng, M);
        if (fail >= need) {
            wall += need;
            done += seg;
        } else {
            wall += fail + R;                                    // work since the checkpoint is lost
        }
    }
    return wall;
}

int main()
{
    std::string label;
    double W, C, R, Mnode, nodes;
    while (std::cin >> label >> W >> C >> R >> Mnode >> nodes) {
        const double M = Mnode / nodes;
        const double best = std::sqrt(2.0 * C * M);
        std::printf("%s: W = %.0f h of work, checkpoint C = %.2f h, restart R = %.2f h,\n", label.c_str(), W, C, R);
        std::printf("  node MTBF %.0f h, %.0f nodes -> job MTBF M = %.2f h; T* = sqrt(2 C M) = %.2f h\n",
                    Mnode, nodes, M, best);
        std::printf("  interval T (h)   mean wall time (h)   efficiency W/wall\n");
        std::vector<double> intervals = {best / 4, best / 2, best, best * 2, best * 4, W};
        for (double T : intervals) {
            std::mt19937_64 rng(12345);                          // same failures for every T
            const int runs = 2000;
            double sum = 0;
            for (int i = 0; i < runs; ++i) sum += run(W, T, C, R, M, rng);
            const double mean = sum / runs;
            std::printf("  %14.2f   %18.1f   %16.3f%s\n", T, mean, W / mean,
                        T == best ? "   <- T*" : (T == W ? "   (never checkpoint)" : ""));
        }
        std::printf("\n");
    }
    return 0;
}

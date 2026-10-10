// F8-12 Listing 2: when does gradient communication hide behind the backward pass?
// A model of one data-parallel training step: L equal layers; the backward pass produces the
// gradients of the last layer first. Gradients are grouped into buckets of k layers; a bucket's
// all-reduce may start when its last layer's gradients exist and when the previous bucket's
// all-reduce has finished (one communication stream). All-reduce time: the ring formula of
// F8-03, 2 (N-1) (alpha + (bytes / N) / beta). All input values are invented teaching values.
// Input: one setup line (layers, MiB per layer, forward us, backward us, ranks, alpha us, beta GB/s,
// label), then commands: "sweep" (a table over bucket sizes) or "trace <layers per bucket> <name>"
// (the timeline of the communication stream, as a profiler would show it).
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

struct Setup
{
    int layers = 0;
    double gradMiB = 0;       // gradient size of one layer
    double fwdUs = 0;         // forward time of one layer
    double bwdUs = 0;         // backward time of one layer
    int ranks = 0;
    double alphaUs = 0;       // fixed cost per message
    double betaGBps = 0;      // link bandwidth (10^9 bytes per second)
};

double ringUs(const Setup& s, double bytes)
{
    const double perMsg = s.alphaUs + bytes / s.ranks / (s.betaGBps * 1e3);   // 1 GB/s = 1e3 B/us
    return 2.0 * (s.ranks - 1) * perMsg;
}

// step time with buckets of k layers; overlap=false waits for the whole backward pass first;
// with trace=true every bucket's ready, start and end times are printed
double stepUs(const Setup& s, int k, bool overlap, double& commUs, bool trace = false)
{
    const double fwd = s.layers * s.fwdUs;
    double commFree = 0, end = 0;
    commUs = 0;
    int b = 0;
    const int buckets = (s.layers + k - 1) / k;
    for (int done = 0; done < s.layers; done += k, ++b) {
        const int n = std::min(k, s.layers - done);
        const double ready = overlap ? fwd + (done + n) * s.bwdUs : fwd + s.layers * s.bwdUs;
        const double t = ringUs(s, n * s.gradMiB * 1048576.0);
        const double start = std::max(commFree, ready);
        commUs += t;
        commFree = start + t;
        end = commFree;
        if (trace && (b < 5 || b >= buckets - 2)) {
            std::printf("  all-reduce %3d  %6.2f MiB  ready %7.0f  start %7.0f  end %7.0f  (queued %5.0f us)\n",
                        b, n * s.gradMiB, ready, start, end, start - ready);
        } else if (trace && b == 5) {
            std::printf("  ... (%d more all-reduces of the same size)\n", buckets - 7);
        }
    }
    return std::max(end, fwd + s.layers * s.bwdUs);
}

int main()
{
    Setup s;
    std::string label;
    std::cin >> s.layers >> s.gradMiB >> s.fwdUs >> s.bwdUs >> s.ranks >> s.alphaUs >> s.betaGBps >> label;
    const double compute = s.layers * (s.fwdUs + s.bwdUs);
    std::printf("%s: %d layers x %.2f MiB of gradients, forward %.0f us + backward %.0f us per layer,\n",
                label.c_str(), s.layers, s.gradMiB, s.fwdUs, s.bwdUs);
    std::printf("N = %d ranks, alpha = %.0f us, beta = %.0f GB/s; compute alone = %.0f us per step\n\n",
                s.ranks, s.alphaUs, s.betaGBps, compute);
    std::string cmd;
    while (std::cin >> cmd) {
        if (cmd == "trace") {
            int k;
            std::string name;
            std::cin >> k >> name;
            double comm = 0;
            std::printf("%s: communication stream of one step (times in us from the start of the step;\n"
                        "  forward ends at %.0f, backward ends at %.0f)\n", name.c_str(), s.layers * s.fwdUs, compute);
            const double t = stepUs(s, k, true, comm, true);
            std::printf("  step time %.0f us; communication busy %.0f us\n\n", t, comm);
            continue;
        }
        std::printf("bucket (layers)  bucket MiB  buckets  all-reduce total us  step us (no overlap)  step us (overlap)  exposed comm us\n");
        double best = 1e300;
        int bestK = 0;
        for (int k : {1, 2, 4, 8, 16, 32, 64}) {
            if (k > s.layers) break;
            double comm = 0;
            const double plain = stepUs(s, k, false, comm);
            const double over = stepUs(s, k, true, comm);
            std::printf("%15d  %10.2f  %7d  %19.0f  %20.0f  %17.0f  %15.0f\n", k, k * s.gradMiB,
                        (s.layers + k - 1) / k, comm, plain, over, over - compute);
            if (over < best) { best = over; bestK = k; }
        }
        std::printf("\nbest bucket: %d layers (%.2f MiB), step %.0f us = %.1f %% of the time the computation alone needs\n\n",
                    bestK, bestK * s.gradMiB, best, 100.0 * best / compute);
    }
    return 0;
}

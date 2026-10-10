// F8-11 Listing 1: hiding gradient all-reduce behind a (simulated) backward pass.
// Each step computes the gradients of kLayers layers from the last layer to the first;
// each layer's gradient is one bucket. Three schedules:
//   serial  - compute every layer, then all-reduce every bucket (blocking MPI_Allreduce);
//   overlap - start MPI_Iallreduce for a bucket as soon as its layer is done, and call
//             MPI_Testsome between slices of the next layer's compute, so the library can
//             make progress; wait for the rest at the end;
//   notest  - like overlap, but no MPI call during compute (progress only in the final wait).
// For every bucket the program records when its all-reduce was started and when this rank
// saw it complete, relative to the start of the step.
// Gradient values are small integers stored as float, so every sum is exact and all three
// schedules must give bit-identical results (checked with a hash).
#include <mpi.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr int kLayers = 8;
constexpr int kBucket = 1 << 19;   // floats per bucket (2 MiB)
constexpr int kSlices = 16;        // compute slices per layer
int kWork = 3;                     // inner work per element (argv[1]): sets the compute time

// The "backward" of one layer: deterministic, costs time, yields small integers.
void computeSlice(std::vector<float>& g, int layer, int rank, int begin, int end)
{
    for (int i = begin; i < end; ++i) {
        double acc = 0.0;
        for (int j = 0; j < kWork; ++j) {
            acc += std::sqrt(static_cast<double>(i + j + layer));
        }
        g[i] = static_cast<float>((static_cast<long>(acc) + rank + layer) % 17);
    }
}

std::uint64_t fnv(const std::vector<std::vector<float>>& b)
{
    std::uint64_t h = 14695981039346656037ULL;
    for (const auto& v : b) {
        for (float f : v) {
            std::uint32_t u = 0;
            std::memcpy(&u, &f, sizeof u);
            h = (h ^ u) * 1099511628211ULL;
        }
    }
    return h;
}

struct Trace
{
    double step = 0.0;         // whole step, ms
    double compute = 0.0;      // time inside computeSlice, ms
    double computeEnd = 0.0;   // when the last layer's compute finished, ms
    std::vector<double> posted = std::vector<double>(kLayers, 0.0);
    std::vector<double> done = std::vector<double>(kLayers, 0.0);
};

// Record completion times of finished requests (MPI sets finished ones to MPI_REQUEST_NULL).
void collect(std::vector<MPI_Request>& req, Trace& tr, double t0, bool block)
{
    std::vector<int> idx(kLayers);
    int outcount = 0;
    if (block) {
        MPI_Waitsome(kLayers, req.data(), &outcount, idx.data(), MPI_STATUSES_IGNORE);
    } else {
        MPI_Testsome(kLayers, req.data(), &outcount, idx.data(), MPI_STATUSES_IGNORE);
    }
    const double now = (MPI_Wtime() - t0) * 1e3;
    for (int k = 0; k < outcount && outcount != MPI_UNDEFINED; ++k) {
        tr.done[idx[k]] = now;
    }
}

Trace runStep(const std::string& mode, int rank, std::vector<std::vector<float>>& grads)
{
    std::vector<MPI_Request> req(kLayers, MPI_REQUEST_NULL);
    const int slice = kBucket / kSlices;
    Trace tr;
    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = MPI_Wtime();
    for (int layer = kLayers - 1; layer >= 0; --layer) {   // backward: last layer first
        for (int s = 0; s < kSlices; ++s) {
            const double c0 = MPI_Wtime();
            computeSlice(grads[layer], layer, rank, s * slice, (s + 1) * slice);
            tr.compute += (MPI_Wtime() - c0) * 1e3;
            if (mode == "overlap") {
                collect(req, tr, t0, false);   // gives the library a chance to progress
            }
        }
        if (mode != "serial") {
            tr.posted[layer] = (MPI_Wtime() - t0) * 1e3;
            MPI_Iallreduce(MPI_IN_PLACE, grads[layer].data(), kBucket, MPI_FLOAT, MPI_SUM,
                           MPI_COMM_WORLD, &req[layer]);
        }
    }
    tr.computeEnd = (MPI_Wtime() - t0) * 1e3;
    if (mode == "serial") {
        for (int layer = kLayers - 1; layer >= 0; --layer) {
            tr.posted[layer] = (MPI_Wtime() - t0) * 1e3;
            MPI_Allreduce(MPI_IN_PLACE, grads[layer].data(), kBucket, MPI_FLOAT, MPI_SUM,
                          MPI_COMM_WORLD);
            tr.done[layer] = (MPI_Wtime() - t0) * 1e3;
        }
    } else {
        while (std::any_of(req.begin(), req.end(), [](MPI_Request r) { return r != MPI_REQUEST_NULL; })) {
            collect(req, tr, t0, true);
        }
    }
    tr.step = (MPI_Wtime() - t0) * 1e3;
    return tr;
}

}  // namespace

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (argc > 1) {
        kWork = std::max(1, std::atoi(argv[1]));
    }
    if (rank == 0) {
        std::printf("work per element %d; ranks %d; %d buckets of %d floats (%d KiB each); "
                    "%d compute slices per layer\n",
                    kWork, size, kLayers, kBucket, kBucket * 4 / 1024, kSlices);
    }
    for (const std::string mode : {"serial", "overlap", "notest"}) {
        std::vector<std::vector<float>> grads(kLayers, std::vector<float>(kBucket));
        runStep(mode, rank, grads);   // warm-up step, not counted
        std::vector<Trace> traces;
        std::vector<double> steps;
        for (int rep = 0; rep < 7; ++rep) {
            traces.push_back(runStep(mode, rank, grads));
            double mx = 0.0;
            MPI_Allreduce(&traces.back().step, &mx, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
            steps.push_back(mx);
        }
        // Show the repetition whose (slowest-rank) step time is the median.
        std::vector<double> sorted = steps;
        std::sort(sorted.begin(), sorted.end());
        const int med = static_cast<int>(std::find(steps.begin(), steps.end(), sorted[3]) - steps.begin());
        const Trace& t = traces[med];
        if (rank == 0) {
            int before = 0;
            for (int k = 0; k < kLayers; ++k) {
                before += (t.done[k] <= t.computeEnd) ? 1 : 0;
            }
            std::printf("\n%s: step %.2f ms (median of 7, slowest rank); rank 0: compute %.2f ms, "
                        "backward pass ended at %.2f ms\n",
                        mode.c_str(), sorted[3], t.compute, t.computeEnd);
            std::printf("  bucket (layer):  ");
            for (int k = kLayers - 1; k >= 0; --k) {
                std::printf("%7d", k);
            }
            std::printf("\n  started at ms:   ");
            for (int k = kLayers - 1; k >= 0; --k) {
                std::printf("%7.1f", t.posted[k]);
            }
            std::printf("\n  completed at ms: ");
            for (int k = kLayers - 1; k >= 0; --k) {
                std::printf("%7.1f", t.done[k]);
            }
            std::printf("\n  buckets complete before the backward pass ended: %d of %d; "
                        "time after it: %.2f ms; result hash %016llx\n",
                        before, kLayers, t.step - t.computeEnd,
                        static_cast<unsigned long long>(fnv(grads)));
        }
    }
    MPI_Finalize();
    return 0;
}

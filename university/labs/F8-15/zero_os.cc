// F8-15 Listing 2: sharding the optimizer state (the idea of ZeRO stage 1) with MPI.
// Two data-parallel trainers of the same linear model run side by side on every rank:
//   plain   - MPI_Allreduce of the gradient, then every rank runs Adam on all P parameters
//             and keeps both Adam moment vectors for all of them;
//   sharded - MPI_Reduce_scatter_block gives each rank the summed gradient of its P/N slice,
//             each rank runs Adam on its slice only (moments for P/N parameters), and
//             MPI_Allgather puts the updated slices back together on every rank.
// The program reports the largest difference between the two and the optimizer memory per rank.
// Argument ckpt=<k>: after step k the sharded trainer saves its state to files, forgets it and
// loads it back (a restart). bug=rank0ckpt saves and loads as if every rank held all the moments
// (only rank 0 writes; every rank reads rank 0's file): the forensic lab's broken version.
#include <mpi.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

constexpr int kP = 1024;       // parameters
constexpr int kBatch = 64;     // global batch
constexpr int kSteps = 50;
constexpr float kLr = 0.01f, kB1 = 0.9f, kB2 = 0.999f, kEps = 1e-8f;
using Vec = std::vector<float>;

std::uint64_t mix(std::uint64_t z)
{
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

float uniform(std::uint64_t key) { return static_cast<float>(mix(key) >> 40) / 8388608.0f - 1.0f; }

// gradient sum of 0.5 * (w.x - y)^2 over samples [first, first+count) of this step
void gradient(const Vec& w, int step, int first, int count, Vec& g)
{
    for (int i = first; i < first + count; ++i) {
        const std::uint64_t base = (static_cast<std::uint64_t>(step) << 40) + static_cast<std::uint64_t>(i) * kP;
        float pred = 0.0f, y = 0.0f;
        for (int j = 0; j < kP; ++j) {
            const float x = uniform(base + j);
            pred += w[j] * x;
            y += (j % 7 == 0 ? 0.5f : 0.0f) * x;          // the true model: every 7th weight is 0.5
        }
        const float d = pred - y;
        for (int j = 0; j < kP; ++j) g[j] += d * uniform(base + j);
    }
}

// Adam on n parameters starting at w[0], with moments m, v of the same length
void adam(float* w, const float* g, float* m, float* v, int n, int t)
{
    const float c1 = 1.0f - std::pow(kB1, static_cast<float>(t));
    const float c2 = 1.0f - std::pow(kB2, static_cast<float>(t));
    for (int j = 0; j < n; ++j) {
        m[j] = kB1 * m[j] + (1.0f - kB1) * g[j];
        v[j] = kB2 * v[j] + (1.0f - kB2) * g[j] * g[j];
        w[j] -= kLr * (m[j] / c1) / (std::sqrt(v[j] / c2) + kEps);
    }
}

void save(const std::string& file, const Vec& a, const Vec& b)
{
    std::FILE* f = std::fopen(file.c_str(), "wb");
    if (!f || std::fwrite(a.data(), sizeof(float), a.size(), f) != a.size() ||
        std::fwrite(b.data(), sizeof(float), b.size(), f) != b.size() || std::fclose(f) != 0) {
        std::printf("cannot write %s\n", file.c_str());
        MPI_Abort(MPI_COMM_WORLD, 3);
    }
}

void load(const std::string& file, Vec& a, Vec& b)
{
    std::FILE* f = std::fopen(file.c_str(), "rb");
    if (!f || std::fread(a.data(), sizeof(float), a.size(), f) != a.size() ||
        std::fread(b.data(), sizeof(float), b.size(), f) != b.size()) {
        std::printf("cannot read %s\n", file.c_str());
        MPI_Abort(MPI_COMM_WORLD, 3);
    }
    std::fclose(f);
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (kP % size != 0 || kBatch % size != 0) {
        if (rank == 0) std::printf("P and the batch must divide by the number of ranks\n");
        MPI_Abort(MPI_COMM_WORLD, 2);
    }
    const int slice = kP / size, shard = kBatch / size;
    int ckptStep = 0;
    bool buggy = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a.rfind("ckpt=", 0) == 0) ckptStep = std::atoi(a.c_str() + 5);
        if (a == "bug=rank0ckpt") buggy = true;
    }
    Vec wPlain(kP, 0.0f), mPlain(kP, 0.0f), vPlain(kP, 0.0f);         // plain: full moments
    Vec wShard(kP, 0.0f), mShard(slice, 0.0f), vShard(slice, 0.0f);   // sharded: P/N moments
    float worst = 0.0f, loss0 = 0.0f, lossEnd = 0.0f, lossShardEnd = 0.0f;
    for (int t = 1; t <= kSteps; ++t) {
        Vec g(kP, 0.0f);
        gradient(wPlain, t, rank * shard, shard, g);
        MPI_Allreduce(MPI_IN_PLACE, g.data(), kP, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
        for (float& x : g) x /= kBatch;
        adam(wPlain.data(), g.data(), mPlain.data(), vPlain.data(), kP, t);

        Vec gs(kP, 0.0f), mine(slice);
        gradient(wShard, t, rank * shard, shard, gs);
        MPI_Reduce_scatter_block(gs.data(), mine.data(), slice, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
        for (float& x : mine) x /= kBatch;
        Vec updated(wShard.begin() + rank * slice, wShard.begin() + (rank + 1) * slice);
        adam(updated.data(), mine.data(), mShard.data(), vShard.data(), slice, t);
        MPI_Allgather(updated.data(), slice, MPI_FLOAT, wShard.data(), slice, MPI_FLOAT, MPI_COMM_WORLD);

        if (t == ckptStep) {                     // save, forget everything, load back
            const std::string mine = buggy ? "os_rank0.bin" : "os_rank" + std::to_string(rank) + ".bin";
            if (!buggy || rank == 0) save(mine, mShard, vShard);
            MPI_Barrier(MPI_COMM_WORLD);         // all files written before anyone reads
            Vec weights = wShard;                // the weights are the same on every rank
            mShard.assign(slice, 0.0f);
            vShard.assign(slice, 0.0f);
            load(mine, mShard, vShard);
            wShard = weights;
            MPI_Barrier(MPI_COMM_WORLD);
            if (rank == 0) std::printf("step %d: sharded optimizer state saved and loaded back%s\n", t,
                                       buggy ? " (rank 0's file only)" : " (one file per rank)");
        }
        float diff = 0.0f;
        for (int j = 0; j < kP; ++j) diff = std::fmax(diff, std::fabs(wPlain[j] - wShard[j]));
        worst = std::fmax(worst, diff);
        if (ckptStep > 0 && rank == 0 && (t == ckptStep || t == ckptStep + 1 || t == kSteps)) {
            std::printf("step %d: max |w_plain - w_sharded| per slice:", t);
            for (int r = 0; r < size; ++r) {
                float d = 0.0f;
                for (int j = r * slice; j < (r + 1) * slice; ++j) d = std::fmax(d, std::fabs(wPlain[j] - wShard[j]));
                std::printf(" slice %d %.3g;", r, d);
            }
            std::printf("\n");
        }
        float err = 0.0f, errShard = 0.0f;                 // distance to the true weights
        for (int j = 0; j < kP; ++j) {
            const float truth = j % 7 == 0 ? 0.5f : 0.0f;
            err += (wPlain[j] - truth) * (wPlain[j] - truth);
            errShard += (wShard[j] - truth) * (wShard[j] - truth);
        }
        if (t == 1) loss0 = err;
        lossEnd = err;
        lossShardEnd = errShard;
    }
    if (rank == 0) {
        std::printf("N = %d ranks, P = %d parameters, slice = %d parameters per rank, %d Adam steps\n",
                    size, kP, slice, kSteps);
        std::printf("|w - w_true|^2 after step 1: %.4f; after step %d: plain %.4f, sharded %.4f\n",
                    loss0, kSteps, lossEnd, lossShardEnd);
        std::printf("optimizer state per rank (two FP32 moments): plain %zu bytes, sharded %zu bytes\n",
                    2 * sizeof(float) * kP, 2 * sizeof(float) * slice);
        std::printf("largest |w_plain - w_sharded| over all steps: %.3g -> %s (tolerance 1e-5)\n",
                    worst, worst <= 1e-5f ? "MATCH" : "DIFFERENT");
    }
    MPI_Barrier(MPI_COMM_WORLD);
    std::remove(("os_rank" + std::to_string(rank) + ".bin").c_str());   // tidy up (may not exist)
    MPI_Finalize();
    return 0;
}

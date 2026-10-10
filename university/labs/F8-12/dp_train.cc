// F8-12 Listing 1: data-parallel training of a tiny MLP with MPI, one process per replica,
// with gradient buckets that are sent while the backward pass is still running.
// Every rank holds a full copy of the parameters. Each step the global batch of kBatch samples
// is cut into equal shards. The backward pass produces the output layer's gradient first: it
// goes out at once as bucket 0 (MPI_Iallreduce, non-blocking), and the hidden layer's gradient
// is computed while bucket 0 travels; then bucket 1 goes out, and the rank waits for both.
// Every rank divides the sums by kBatch and applies the same SGD update.
// Rank 0 also trains a single-process reference on the whole batch and compares the two.
#include <mpi.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

constexpr int kIn = 4;          // inputs per sample
constexpr int kHidden = 8;      // hidden units
constexpr int kBatch = 32;      // global batch size (samples per step, all ranks together)
constexpr int kSteps = 60;
constexpr float kLr = 0.05f;
// bucket 0: output layer w2[kHidden], b2, and the loss sum; bucket 1: W1[kHidden][kIn], b1[kHidden]
constexpr int kOut = kHidden + 2;
constexpr int kHid = kHidden * kIn + kHidden;
using Vec = std::vector<float>;

struct Model
{
    Vec w1 = Vec(kHidden * kIn), b1 = Vec(kHidden), w2 = Vec(kHidden);
    float b2 = 0.0f;
};

std::uint64_t mix(std::uint64_t z)     // splitmix64: a deterministic hash
{
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

float uniform(std::uint64_t key)        // in [-1, 1), a pure function of key
{
    return static_cast<float>(mix(key) >> 40) / 8388608.0f - 1.0f;
}

// sample i of step s: the same numbers on every rank, whoever owns the sample
void sample(int step, int i, float x[kIn], float& y)
{
    for (int k = 0; k < kIn; ++k) x[k] = uniform((static_cast<std::uint64_t>(step) << 32) + i * 8 + k);
    y = std::sin(2.0f * x[0]) + 0.5f * x[1] * x[2] - 0.3f * x[3];
}

Model initModel()
{
    Model m;
    int j = 0;
    for (float& v : m.w1) v = 0.5f * uniform((1ull << 40) + j++);
    for (float& v : m.b1) v = 0.5f * uniform((1ull << 40) + j++);
    for (float& v : m.w2) v = 0.5f * uniform((1ull << 40) + j++);
    m.b2 = 0.5f * uniform((1ull << 40) + j);
    return m;
}

struct Saved                           // what the forward pass keeps for the backward pass
{
    Vec x, h, d;                       // inputs, hidden activations, output error per sample
};

Saved forward(const Model& m, int step, int first, int count)
{
    Saved s{Vec(count * kIn), Vec(count * kHidden), Vec(count)};
    for (int n = 0; n < count; ++n) {
        float y;
        sample(step, first + n, &s.x[n * kIn], y);
        float out = m.b2;
        for (int u = 0; u < kHidden; ++u) {
            float z = m.b1[u];
            for (int k = 0; k < kIn; ++k) z += m.w1[u * kIn + k] * s.x[n * kIn + k];
            s.h[n * kHidden + u] = std::tanh(z);
            out += m.w2[u] * s.h[n * kHidden + u];
        }
        s.d[n] = out - y;              // dLoss/dout for loss = d*d/2
    }
    return s;
}

Vec backwardOutput(const Saved& s)     // bucket 0: sums of dw2, db2 and the loss
{
    Vec g(kOut, 0.0f);
    for (std::size_t n = 0; n < s.d.size(); ++n) {
        for (int u = 0; u < kHidden; ++u) g[u] += s.d[n] * s.h[n * kHidden + u];
        g[kHidden] += s.d[n];
        g[kHidden + 1] += 0.5f * s.d[n] * s.d[n];
    }
    return g;
}

Vec backwardHidden(const Model& m, const Saved& s)   // bucket 1: sums of dW1 and db1
{
    Vec g(kHid, 0.0f);
    for (std::size_t n = 0; n < s.d.size(); ++n) {
        for (int u = 0; u < kHidden; ++u) {
            const float hu = s.h[n * kHidden + u];
            const float dz = s.d[n] * m.w2[u] * (1.0f - hu * hu);
            for (int k = 0; k < kIn; ++k) g[u * kIn + k] += dz * s.x[n * kIn + k];
            g[kHidden * kIn + u] += dz;
        }
    }
    return g;
}

void update(Model& m, const Vec& g0, const Vec& g1)
{
    const float a = kLr / kBatch;      // the sums cover the whole global batch: divide once
    for (int u = 0; u < kHidden; ++u) m.w2[u] -= a * g0[u];
    m.b2 -= a * g0[kHidden];
    for (int j = 0; j < kHidden * kIn; ++j) m.w1[j] -= a * g1[j];
    for (int u = 0; u < kHidden; ++u) m.b1[u] -= a * g1[kHidden * kIn + u];
}

Vec flat(const Model& m)
{
    Vec v = m.w1;
    v.insert(v.end(), m.b1.begin(), m.b1.end());
    v.insert(v.end(), m.w2.begin(), m.w2.end());
    v.push_back(m.b2);
    return v;
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (kBatch % size != 0) {
        if (rank == 0) std::printf("global batch %d does not split into %d equal shards\n", kBatch, size);
        MPI_Abort(MPI_COMM_WORLD, 2);
    }
    const int shard = kBatch / size;
    Model m = initModel();
    MPI_Bcast(m.w1.data(), kHidden * kIn, MPI_FLOAT, 0, MPI_COMM_WORLD);   // one starting point
    MPI_Bcast(m.b1.data(), kHidden, MPI_FLOAT, 0, MPI_COMM_WORLD);         // for all replicas
    MPI_Bcast(m.w2.data(), kHidden, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&m.b2, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
    Model ref = initModel();                                               // rank 0's reference
    if (rank == 0) {
        std::printf("ranks %d, shard %d samples per rank, global batch %d; bucket 0: %d values, bucket 1: %d values\n",
                    size, shard, kBatch, kOut, kHid);
        std::printf("step   loss(data-parallel)  loss(reference)  max|p - p_ref|  replica spread\n");
    }
    float worst = 0.0f;
    for (int step = 0; step < kSteps; ++step) {
        const Saved s = forward(m, step, rank * shard, shard);
        Vec g0 = backwardOutput(s);
        MPI_Request req[2];
        MPI_Iallreduce(MPI_IN_PLACE, g0.data(), kOut, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD, &req[0]);
        Vec g1 = backwardHidden(m, s);                 // computed while bucket 0 is in flight
        MPI_Iallreduce(MPI_IN_PLACE, g1.data(), kHid, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD, &req[1]);
        MPI_Waitall(2, req, MPI_STATUSES_IGNORE);
        update(m, g0, g1);

        // replica spread: the largest difference of one parameter between two ranks (0 expected)
        const Vec mine = flat(m);
        Vec hi = mine, lo = mine;
        MPI_Allreduce(MPI_IN_PLACE, hi.data(), static_cast<int>(hi.size()), MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, lo.data(), static_cast<int>(lo.size()), MPI_FLOAT, MPI_MIN, MPI_COMM_WORLD);
        float spread = 0.0f;
        for (std::size_t j = 0; j < hi.size(); ++j) spread = std::fmax(spread, hi[j] - lo[j]);

        if (rank == 0) {
            const Saved r = forward(ref, step, 0, kBatch);
            const Vec r0 = backwardOutput(r), r1 = backwardHidden(ref, r);
            update(ref, r0, r1);
            const Vec a = flat(m), b = flat(ref);
            float diff = 0.0f;
            for (std::size_t j = 0; j < a.size(); ++j) diff = std::fmax(diff, std::fabs(a[j] - b[j]));
            worst = std::fmax(worst, diff);
            if (step % 10 == 0 || step == kSteps - 1) {
                std::printf("%4d   %19.7f  %15.7f  %14.3g  %14.3g\n",
                            step, g0[kHidden + 1] / kBatch, r0[kHidden + 1] / kBatch, diff, spread);
            }
        }
    }
    if (rank == 0) {
        std::printf("largest |p - p_ref| over all steps: %.3g -> %s (tolerance 1e-5)\n",
                    worst, worst <= 1e-5f ? "MATCH" : "DIFFERENT");
    }
    MPI_Finalize();
    return 0;
}

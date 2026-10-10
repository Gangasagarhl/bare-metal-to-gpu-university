// F8-10 Listing 1: a hierarchical all-reduce built from MPI communicators, checked against
// MPI_Allreduce. Usage: hier_allreduce <ranks per node> [nocheck]
// All processes run on one machine here, so "nodes" are simulated: node = rank / ppn.
// Algorithm (when every node has the same number of ranks):
//   1. intra-node reduce-scatter: local rank j ends with shard j of its node's sum;
//   2. inter-node all-reduce of shard j among the ranks with local rank j (one per node);
//   3. intra-node all-gather of the shards.
// If node sizes differ, the checked version falls back to: reduce to the node leader,
// all-reduce among leaders, broadcast inside the node. "nocheck" skips that test (the bug
// of the forensic lab).
#include <mpi.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

struct Comms
{
    MPI_Comm node;    // ranks on my node
    MPI_Comm cross;   // ranks with my local rank, one per node
    int localRank;
    int localSize;
    bool uniform;     // every node has the same number of ranks
};

Comms makeComms(int rank, int ppn)
{
    Comms c{};
    MPI_Comm_split(MPI_COMM_WORLD, rank / ppn, rank, &c.node);
    MPI_Comm_rank(c.node, &c.localRank);
    MPI_Comm_size(c.node, &c.localSize);
    MPI_Comm_split(MPI_COMM_WORLD, c.localRank, rank, &c.cross);
    int minSize = 0;
    int maxSize = 0;
    MPI_Allreduce(&c.localSize, &minSize, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    MPI_Allreduce(&c.localSize, &maxSize, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    c.uniform = (minSize == maxSize);
    return c;
}

// Hierarchical sum of in[0..n) into out[0..n) for one MPI datatype.
template <typename T>
void hierAllreduce(const std::vector<T>& in, std::vector<T>& out, MPI_Datatype type,
                   const Comms& c, int ppn, bool checked)
{
    const int n = static_cast<int>(in.size());
    if (checked && !c.uniform) {
        // Leader-based fallback: correct for any node sizes.
        std::vector<T> nodeSum(n);
        MPI_Reduce(in.data(), nodeSum.data(), n, type, MPI_SUM, 0, c.node);
        if (c.localRank == 0) {
            MPI_Allreduce(MPI_IN_PLACE, nodeSum.data(), n, type, MPI_SUM, c.cross);
        }
        MPI_Bcast(nodeSum.data(), n, type, 0, c.node);
        out = nodeSum;
        return;
    }
    // Shards: pad n up to a multiple of the node size so every shard has the same length.
    // The unchecked version uses the ppn from the command line, assuming every node is full.
    const int parts = checked ? c.localSize : ppn;
    const int shard = (n + parts - 1) / parts;
    std::vector<T> padded(static_cast<std::size_t>(shard) * parts, T{});
    std::memcpy(padded.data(), in.data(), sizeof(T) * n);
    std::vector<T> mine(shard);
    MPI_Reduce_scatter_block(padded.data(), mine.data(), shard, type, MPI_SUM, c.node);   // step 1
    MPI_Allreduce(MPI_IN_PLACE, mine.data(), shard, type, MPI_SUM, c.cross);              // step 2
    std::vector<T> gathered(static_cast<std::size_t>(shard) * parts, T{});
    MPI_Allgather(mine.data(), shard, type, gathered.data(), shard, type, c.node);         // step 3
    out.assign(gathered.begin(), gathered.begin() + n);
}

}  // namespace

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    const int ppn = argc > 1 ? std::atoi(argv[1]) : 2;
    const bool checked = !(argc > 2 && std::strcmp(argv[2], "nocheck") == 0);

    // What the library says about real nodes (all processes share this machine).
    MPI_Comm shared;
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &shared);
    int sharedSize = 0;
    MPI_Comm_size(shared, &sharedSize);

    const Comms c = makeComms(rank, ppn);
    if (rank == 0) {
        std::printf("world %d ranks; MPI_COMM_TYPE_SHARED groups %d ranks per real node; "
                    "simulated ppn %d; node sizes %s; %s\n",
                    size, sharedSize, ppn, c.uniform ? "equal" : "UNEQUAL",
                    checked ? "checked" : "nocheck");
    }

    int allOk = 1;
    for (int n : {1, 7, 1000, 1 << 20}) {
        // Integer data: rank r contributes (r + 1) * (i % 7 + 1); exact expected sum.
        std::vector<std::int64_t> in(n);
        for (int i = 0; i < n; ++i) {
            in[i] = static_cast<std::int64_t>(rank + 1) * (i % 7 + 1);
        }
        std::vector<std::int64_t> hier;
        hierAllreduce(in, hier, MPI_INT64_T, c, ppn, checked);
        const std::int64_t tri = static_cast<std::int64_t>(size) * (size + 1) / 2;
        // Count wrong elements per quarter of the vector, then combine over ranks.
        long bad[4] = {0, 0, 0, 0};
        for (int i = 0; i < n; ++i) {
            if (hier[i] != tri * (i % 7 + 1)) {
                ++bad[static_cast<long>(i) * 4 / n];
            }
        }
        long badAll[4];
        MPI_Reduce(bad, badAll, 4, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        // Float data: compare with the library's flat all-reduce (different summation order).
        std::vector<float> fin(n);
        for (int i = 0; i < n; ++i) {
            fin[i] = 1.0f / static_cast<float>(1 + (rank * 7919 + i) % 1000);
        }
        std::vector<float> fh;
        hierAllreduce(fin, fh, MPI_FLOAT, c, ppn, checked);
        std::vector<float> ff(n);
        MPI_Allreduce(fin.data(), ff.data(), n, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
        double maxRel = 0.0;
        long bitsDiffer = 0;
        for (int i = 0; i < n; ++i) {
            maxRel = std::fmax(maxRel, std::fabs(static_cast<double>(fh[i]) - ff[i]) / ff[i]);
            bitsDiffer += (fh[i] != ff[i]) ? 1 : 0;
        }
        double maxRelAll = 0.0;
        long bitsAll = 0;
        MPI_Reduce(&maxRel, &maxRelAll, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&bitsDiffer, &bitsAll, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        if (rank == 0) {
            const bool ok = (badAll[0] + badAll[1] + badAll[2] + badAll[3]) == 0;
            allOk = allOk && ok;
            std::printf("  n=%-8d int64 %s (wrong elements per quarter, all ranks: %ld %ld %ld %ld)"
                        "  float vs flat: max rel diff %.2e, %ld of %ld values differ in bits\n",
                        n, ok ? "PASS" : "FAIL", badAll[0], badAll[1], badAll[2], badAll[3],
                        maxRelAll, bitsAll, static_cast<long>(n) * size);
        }
        if (!checked && n == 1000) {
            // Forensic detail: what each rank holds at element 3 (expected tri * 4).
            std::int64_t v = hier[3];
            std::vector<std::int64_t> all(size);
            MPI_Gather(&v, 1, MPI_INT64_T, all.data(), 1, MPI_INT64_T, 0, MPI_COMM_WORLD);
            std::int64_t w = hier[n - 1];   // n - 1 = 999, 999 % 7 + 1 = 6
            std::vector<std::int64_t> allW(size);
            MPI_Gather(&w, 1, MPI_INT64_T, allW.data(), 1, MPI_INT64_T, 0, MPI_COMM_WORLD);
            if (rank == 0) {
                for (int r = 0; r < size; ++r) {
                    std::printf("    rank %d (node %d): element 3 = %lld (expected %lld), "
                                "element 999 = %lld (expected %lld)\n",
                                r, r / ppn, static_cast<long long>(all[r]),
                                static_cast<long long>(tri * 4), static_cast<long long>(allW[r]),
                                static_cast<long long>(tri * 6));
                }
            }
        }
    }
    if (rank == 0) {
        std::printf("result: %s\n", allOk ? "all integer checks PASS" : "integer checks FAILED");
    }
    MPI_Comm_free(&shared);
    MPI_Finalize();
    return 0;
}

// F8-01 forensic evidence: "the job that never finished".
// Every rank sums the items of its shard, then all ranks add their sums with MPI_Allreduce.
// The planted bug: a rank whose shard is empty "has nothing to contribute" and skips the
// collective. Built with mpic++ and run by run.sh under a 10 s time limit.
#include <mpi.h>
#include <cstdio>
#include <vector>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int items = 9;                        // 9 items over 4 ranks: 3, 3, 3, 0
    const int perRank = (items + size - 1) / size;
    const int first = rank * perRank;
    const int last = (first + perRank < items) ? first + perRank : items;
    std::vector<long> shard;
    for (int i = first; i < last; ++i) {
        shard.push_back(i + 1);                 // the items are 1..9
    }

    long local = 0;
    for (long v : shard) {
        local += v;
    }
    long total = 0;
    std::printf("rank %d: shard has %zu items, local sum %ld\n", rank, shard.size(), local);
    std::fflush(stdout);
    if (!shard.empty()) {                       // BUG: a collective must be called by every rank
        std::printf("rank %d: entering MPI_Allreduce\n", rank);
        std::fflush(stdout);
        MPI_Allreduce(&local, &total, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);
        std::printf("rank %d: total %ld\n", rank, total);
    } else {
        std::printf("rank %d: empty shard, skipping MPI_Allreduce\n", rank);
        std::fflush(stdout);
        MPI_Barrier(MPI_COMM_WORLD);            // "wait for the others before finishing"
        std::printf("rank %d: past the barrier\n", rank);
    }
    std::fflush(stdout);
    MPI_Finalize();
    return 0;
}

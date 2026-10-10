// F8-01 Listing 2: the same six collectives, called through a real library (Open MPI).
// Four processes on one machine. Built with mpic++ and started by run.sh with mpirun.
// Each rank checks its own result against the definition in Listing 1; rank 0 gathers
// every rank's result so that the printout is in rank order.
#include <mpi.h>
#include <cstdio>
#include <string>
#include <vector>

constexpr int COUNT = 4;   // elements per rank, as in Listing 1

// rank 0 prints one line per rank; every rank passes its local result
void report(const char* title, const std::vector<int>& mine, int rank, int size)
{
    const int n = static_cast<int>(mine.size());
    std::vector<int> all(rank == 0 ? static_cast<std::size_t>(n * size) : 0);
    MPI_Gather(mine.data(), n, MPI_INT, all.data(), n, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        std::printf("%s\n", title);
        for (int r = 0; r < size; ++r) {
            std::string line = "  rank " + std::to_string(r) + ":";
            for (int i = 0; i < n; ++i) {
                line += " " + std::to_string(all[static_cast<std::size_t>(r * n + i)]);
            }
            std::printf("%s\n", line.c_str());
        }
    }
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::vector<int> x(COUNT);
    for (int i = 0; i < COUNT; ++i) {
        x[static_cast<std::size_t>(i)] = 10 * rank + i;
    }
    int bad = 0;   // collectives whose result differs from the definition, on this rank

    std::vector<int> b = x;                                   // broadcast works in place
    MPI_Bcast(b.data(), COUNT, MPI_INT, 0, MPI_COMM_WORLD);
    for (int i = 0; i < COUNT; ++i) { bad += (b[static_cast<std::size_t>(i)] != i); }
    report("MPI_Bcast from root 0:", b, rank, size);

    std::vector<int> red(COUNT, -1);                          // only the root's result is defined
    MPI_Reduce(x.data(), red.data(), COUNT, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank != 0) { red = x; }                               // print the unchanged input instead
    report("MPI_Reduce (MPI_SUM) to root 0 (other ranks show their unchanged input):", red, rank, size);

    std::vector<int> ar(COUNT);
    MPI_Allreduce(x.data(), ar.data(), COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    for (int i = 0; i < COUNT; ++i) {
        const int expect = 10 * (size * (size - 1) / 2) + size * i;
        bad += (ar[static_cast<std::size_t>(i)] != expect);
    }
    report("MPI_Allreduce (MPI_SUM):", ar, rank, size);

    std::vector<int> rs(static_cast<std::size_t>(COUNT / size));   // one block per rank
    MPI_Reduce_scatter_block(x.data(), rs.data(), COUNT / size, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    report("MPI_Reduce_scatter_block (MPI_SUM), 1 element per rank:", rs, rank, size);

    std::vector<int> ag(static_cast<std::size_t>(COUNT * size));
    MPI_Allgather(x.data(), COUNT, MPI_INT, ag.data(), COUNT, MPI_INT, MPI_COMM_WORLD);
    report("MPI_Allgather (every rank's 4 elements, in rank order):", ag, rank, size);

    std::vector<int> a2a(COUNT);
    MPI_Alltoall(x.data(), COUNT / size, MPI_INT, a2a.data(), COUNT / size, MPI_INT, MPI_COMM_WORLD);
    for (int j = 0; j < size; ++j) {                          // block j came from rank j
        bad += (a2a[static_cast<std::size_t>(j)] != 10 * j + rank);
    }
    report("MPI_Alltoall (block j of rank i goes to rank j):", a2a, rank, size);

    int totalBad = 0;
    MPI_Allreduce(&bad, &totalBad, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (rank == 0) {
        std::printf("ranks: %d; results that differ from the definitions: %d\n", size, totalBad);
    }
    MPI_Finalize();
    return totalBad == 0 ? 0 : 1;
}

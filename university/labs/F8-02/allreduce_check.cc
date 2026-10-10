// F8-02 Listing 2: one MPI_Allreduce, checked and timed, so that run.sh can force each of
// Open MPI's all-reduce algorithms in turn and show that they all give the same answer.
// Built with mpic++ and started by run.sh with mpirun (algorithm chosen with --mca options).
#include <mpi.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    const int count = (argc > 1) ? std::atoi(argv[1]) : (1 << 20);    // floats per rank

    // integer-valued floats: every partial sum is exact, so any summation order is exact too
    std::vector<float> in(static_cast<std::size_t>(count)), out(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        in[static_cast<std::size_t>(i)] = static_cast<float>((rank + 1) * (i % 1000));
    }
    const int reps = 10;
    std::vector<double> ms;
    for (int rep = 0; rep < reps; ++rep) {
        MPI_Barrier(MPI_COMM_WORLD);                    // start together
        const double t0 = MPI_Wtime();
        MPI_Allreduce(in.data(), out.data(), count, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
        ms.push_back((MPI_Wtime() - t0) * 1e3);
    }
    long wrong = 0;
    const int ranksSum = size * (size + 1) / 2;         // 1 + 2 + ... + size
    for (int i = 0; i < count; ++i) {
        wrong += (out[static_cast<std::size_t>(i)] != static_cast<float>(ranksSum * (i % 1000)));
    }
    long allWrong = 0;
    MPI_Reduce(&wrong, &allWrong, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    std::sort(ms.begin(), ms.end());
    if (rank == 0) {
        std::printf("ranks %d, %d floats: wrong elements %ld, median of %d: %.3f ms\n",
                    size, count, allWrong, reps, ms[static_cast<std::size_t>(reps / 2)]);
    }
    MPI_Finalize();
    return allWrong == 0 ? 0 : 1;
}

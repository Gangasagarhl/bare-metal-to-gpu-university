// F8-06 Listing 2: the same training step with a real communication library (Open MPI),
// one process per worker: the call pattern a data-parallel step uses with NCCL or RCCL.
// (1) every rank calls every collective, in the same order, with the same count and type;
// (2) rank 0's initial parameters are broadcast; (3) one in-place all-reduce per step adds
// the gradients and the loss sums. Built with mpic++ and run by run.sh with 1, 2 and 4 ranks.
#include <mpi.h>
#include <cstdio>
#include <vector>
#include "mlp.hpp"

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    const std::uint64_t batch = 64;
    const float lr = 0.05f;
    const int steps = 300;
    std::vector<float> p = initParams(rank == 0 ? 42u : 0u);   // only rank 0's values matter
    MPI_Bcast(p.data(), static_cast<int>(NPARAM), MPI_FLOAT, 0, MPI_COMM_WORLD);
    const std::uint64_t r = static_cast<std::uint64_t>(rank), n = static_cast<std::uint64_t>(size);
    for (int s = 0; s < steps; ++s) {
        std::vector<float> buf(NPARAM + 1, 0.0f);
        const std::uint64_t base = static_cast<std::uint64_t>(s) * batch;
        buf[NPARAM] = lossAndGrad(p, base + r * batch / n, base + (r + 1) * batch / n, buf);
        for (std::size_t i = 0; i < NPARAM; ++i) {
            buf[i] /= static_cast<float>(batch);                // global mean, not local
        }
        MPI_Allreduce(MPI_IN_PLACE, buf.data(), static_cast<int>(NPARAM + 1), MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
        for (std::size_t i = 0; i < NPARAM; ++i) {
            p[i] -= lr * buf[i];
        }
        if (rank == 0 && (s % 50 == 0 || s == steps - 1)) {
            std::printf("ranks %d step %3d loss %.6f\n", size, s, static_cast<double>(buf[NPARAM] / batch));
        }
    }
    // replicas must be identical: compare every rank's parameters with rank 0's
    std::vector<float> p0 = p;
    MPI_Bcast(p0.data(), static_cast<int>(NPARAM), MPI_FLOAT, 0, MPI_COMM_WORLD);
    int differ = (p0 != p) ? 1 : 0, anyDiffer = 0;
    MPI_Reduce(&differ, &anyDiffer, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        std::printf("ranks %d: ranks whose parameters differ from rank 0: %d\n", size, anyDiffer);
    }
    MPI_Finalize();
    return anyDiffer == 0 ? 0 : 1;
}

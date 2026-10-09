// pi_mpi.cc - one job, several processes: each rank integrates part of
// 4/(1+x^2) on [0,1] and rank 0 adds the parts (F5-22, DS303).
// Built with mpic++ and started with mpirun by run.sh (not by g++ directly).
#include <mpi.h>
#include <cstdio>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    char host[MPI_MAX_PROCESSOR_NAME] = {};
    int hostLen = 0;
    MPI_Get_processor_name(host, &hostLen);

    const long steps = 400000000;
    const double h = 1.0 / static_cast<double>(steps);
    const double t0 = MPI_Wtime();
    double part = 0.0;
    for (long i = rank; i < steps; i += size) {  // rank r takes every size-th strip
        const double x = (static_cast<double>(i) + 0.5) * h;
        part += 4.0 / (1.0 + x * x);
    }
    part *= h;
    const double computeSeconds = MPI_Wtime() - t0;

    double pi = 0.0;
    MPI_Reduce(&part, &pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    std::printf("rank %d of %d on host %s: my part = %.12f, computed in %.2f s\n", rank, size,
                host, part, computeSeconds);
    if (rank == 0) {
        std::printf("sum of parts = %.12f\n", pi);
    }
    MPI_Finalize();
    return 0;
}

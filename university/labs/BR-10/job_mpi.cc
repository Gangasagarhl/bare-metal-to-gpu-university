// job_mpi.cc - BR-10 Listing 4: the same job as MPI ranks (the launcher picks the nodes).
// Usage: mpirun -np 3 ./job_mpi [stall]   ("stall": rank 1 never reaches the reduction)
#include "job.hpp"
#include <mpi.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0, size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    char host[MPI_MAX_PROCESSOR_NAME];
    int len = 0;
    MPI_Get_processor_name(host, &len);
    bool const stall = argc > 1 && std::strcmp(argv[1], "stall") == 0;

    std::uint64_t const end = 300000;
    int const tasks = 12;
    double const t0 = MPI_Wtime();
    job::Part mine;
    int mineTasks = 0;
    for (int t = rank; t < tasks; t += size) {  // fixed assignment: task t to rank t % size
        job::Part const p = job::countPrimes(job::taskBegin(t, tasks, end),
                                             job::taskBegin(t + 1, tasks, end));
        mine.count += p.count;
        mine.sum += p.sum;
        ++mineTasks;
    }
    std::printf("rank %d of %d on host %s: %d tasks, count %llu, %.1f ms\n", rank, size, host,
                mineTasks, static_cast<unsigned long long>(mine.count), (MPI_Wtime() - t0) * 1e3);
    std::fflush(stdout);
    if (stall && rank == 1) {
        std::printf("rank 1: stalling before the reduction (injected fault)\n");
        std::fflush(stdout);
        while (true) {
            ::sleep(1);
        }
    }
    unsigned long long local[2] = {mine.count, mine.sum}, total[2] = {0, 0};
    MPI_Reduce(local, total, 2, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        job::Part const answer = job::countPrimes(0, end);
        std::printf("rank 0: total count %llu, sum %llu; laptop answer %s\n", total[0], total[1],
                    total[0] == answer.count && total[1] == answer.sum ? "matches" : "DIFFERS");
    }
    MPI_Finalize();
    return 0;
}

// BR-04 Listing 6: one process fails, the whole job stops. Four ranks run ten "training
// steps", each ending with an all-reduce of a small gradient. Rank 2 crashes during step 4
// (it raises SIGKILL on itself, as if the machine or the driver had killed it).
// Usage (run.sh): mpirun -np 4 ./fail_one   (run under a time limit)
#include <mpi.h>
#include <csignal>
#include <cstdio>
#include <vector>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int me = 0;
    int n = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &n);
    std::vector<float> grad(1024, 1.0f);
    for (int step = 1; step <= 10; ++step) {
        if (me == 2 && step == 4) {
            std::printf("rank 2: crashing in step 4\n");
            std::fflush(stdout);
            std::raise(SIGKILL);
        }
        MPI_Allreduce(MPI_IN_PLACE, grad.data(), static_cast<int>(grad.size()), MPI_FLOAT, MPI_SUM,
                      MPI_COMM_WORLD);
        if (me == 0) {
            std::printf("rank 0: step %d done\n", step);
            std::fflush(stdout);
        }
    }
    MPI_Finalize();
    return 0;
}

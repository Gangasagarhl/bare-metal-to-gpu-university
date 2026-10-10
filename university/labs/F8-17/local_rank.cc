// F8-17 Listing 5: which GPU should this rank use? A launcher starts one process per GPU on
// every node; each process finds its node-local rank and picks that GPU. MPI_Comm_split_type
// with MPI_COMM_TYPE_SHARED groups the processes that share a node (they can share memory).
// The program also prints the variable Open MPI's launcher sets for the same purpose.
// Argument: gpus=<GPUs per node> (default 4).
#include <mpi.h>

#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int gpus = 4;
    if (argc > 1 && std::string(argv[1]).rfind("gpus=", 0) == 0) gpus = std::atoi(argv[1] + 5);

    MPI_Comm node;
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &node);
    int local = 0, localSize = 1;
    MPI_Comm_rank(node, &local);
    MPI_Comm_size(node, &localSize);
    const char* env = std::getenv("OMPI_COMM_WORLD_LOCAL_RANK");
    std::printf("rank %d of %d: node-local rank %d of %d (OMPI_COMM_WORLD_LOCAL_RANK=%s) -> GPU %d%s\n",
                rank, size, local, localSize, env ? env : "(not set)", local % gpus,
                localSize > gpus ? "  WARNING: more processes than GPUs on this node" : "");
    MPI_Comm_free(&node);
    MPI_Finalize();
    return 0;
}

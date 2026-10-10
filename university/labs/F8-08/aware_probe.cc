// F8-08 Listing 1: ask the MPI library whether it can take GPU (device) pointers.
// Open MPI answers twice: at compile time (the macro MPIX_CUDA_AWARE_SUPPORT in mpi-ext.h)
// and at run time (MPIX_Query_cuda_support()). Both come from Open MPI's "cuda" extension;
// other MPI libraries have other mechanisms (see the chapter).
#include <mpi.h>
#include <mpi-ext.h>   // Open MPI extensions: declares the CUDA query when built

#include <cstdio>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        char version[MPI_MAX_LIBRARY_VERSION_STRING];
        int len = 0;
        MPI_Get_library_version(version, &len);
        std::printf("library: %.*s\n", len > 60 ? 60 : len, version);

#if defined(MPIX_CUDA_AWARE_SUPPORT)
        std::printf("compile time: MPIX_CUDA_AWARE_SUPPORT = %d\n", MPIX_CUDA_AWARE_SUPPORT);
#else
        std::printf("compile time: MPIX_CUDA_AWARE_SUPPORT is not defined\n");
#endif

#if defined(OMPI_HAVE_MPI_EXT_CUDA) && OMPI_HAVE_MPI_EXT_CUDA
        const int runtime = MPIX_Query_cuda_support();
        std::printf("run time:     MPIX_Query_cuda_support() = %d\n", runtime);
#else
        const int runtime = 0;
        std::printf("run time:     no CUDA extension in this MPI library\n");
#endif
        std::printf("decision:     %s\n",
                    runtime == 1 ? "pass device pointers to MPI calls"
                                 : "stage device buffers through host memory yourself");
    }
    MPI_Finalize();
    return 0;
}

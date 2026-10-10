// F8-17 Listing 6: the first lines of a GPU training process: choose the GPU from the
// node-local rank, check the choice, and fail loudly if it is impossible.
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

static bool check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::printf("%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        return false;
    }
    return true;
}

int main()
{
    const char* env = std::getenv("OMPI_COMM_WORLD_LOCAL_RANK");
    const int local = env ? std::atoi(env) : 0;
    int count = 0;
    if (!check(cudaGetDeviceCount(&count), "cudaGetDeviceCount")) return 1;
    if (local >= count) {
        std::printf("local rank %d but only %d visible GPUs: wrong job shape\n", local, count);
        return 1;
    }
    if (!check(cudaSetDevice(local), "cudaSetDevice")) return 1;
    std::printf("local rank %d uses GPU %d of %d\n", local, local, count);
    return 0;
}

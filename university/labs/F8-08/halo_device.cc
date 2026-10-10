// F8-08 Listing 3: the 1-D heat stencil of F8-07 Listing 2, with the grid in GPU memory.
// CUDA source: compiled with "nvcc -x cu" plus the MPI flags (the .cc name only keeps the
// generic lab runner from building it without MPI). One GPU per rank.
// The halo exchange takes one of two paths, chosen at run time:
//   aware  - the MPI library accepts device pointers: pass them straight to MPI_Sendrecv;
//   staged - copy the boundary cells to pinned host buffers, exchange, copy the halos back.
#include <mpi.h>
#include <mpi-ext.h>
#include <cuda_runtime.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

constexpr int kCells = 1000;
constexpr int kSteps = 500;
constexpr double kR = 0.25;

void cudaCheck(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "CUDA error in %s: %s (%s)\n", what, cudaGetErrorName(err),
                     cudaGetErrorString(err));
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

void mpiCheck(int rc, const char* what)
{
    if (rc != MPI_SUCCESS) {
        std::fprintf(stderr, "MPI error in %s: code %d\n", what, rc);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

// Same arithmetic, in the same order, as the CPU version. The _rn intrinsics are never
// contracted into a fused multiply-add, so the result can be bit-identical to the CPU's.
__global__ void heatStep(const double* u, double* next, int local)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x + 1;   // owned cells are 1..local
    if (i <= local) {
        const double lap = __dadd_rn(__dsub_rn(u[i - 1], __dmul_rn(2.0, u[i])), u[i + 1]);
        next[i] = __dadd_rn(u[i], __dmul_rn(kR, lap));
    }
}

std::uint64_t fnv1a(const std::vector<double>& v)
{
    std::uint64_t h = 14695981039346656037ULL;
    for (double d : v) {
        unsigned char b[sizeof(double)];
        std::memcpy(b, &d, sizeof(double));
        for (unsigned char c : b) {
            h ^= c;
            h *= 1099511628211ULL;
        }
    }
    return h;
}

}  // namespace

int main(int argc, char** argv)
{
    mpiCheck(MPI_Init(&argc, &argv), "MPI_Init");
    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Decide the path once. "--force-staged" lets you compare both paths on an aware MPI.
    bool aware = false;
#if defined(MPIX_CUDA_AWARE_SUPPORT) && MPIX_CUDA_AWARE_SUPPORT
    aware = (MPIX_Query_cuda_support() == 1);
#endif
    if (argc > 1 && std::strcmp(argv[1], "--force-staged") == 0) {
        aware = false;
    }
    if (rank == 0) {
        std::printf("halo path: %s\n", aware ? "aware (device pointers to MPI)" : "staged");
        std::fflush(stdout);
    }

    // One GPU per rank: pick a device by rank (a node-local rank is better; see F8-10).
    int devices = 0;
    cudaCheck(cudaGetDeviceCount(&devices), "cudaGetDeviceCount");
    cudaCheck(cudaSetDevice(rank % devices), "cudaSetDevice");

    const int base = kCells / size;
    const int extra = kCells % size;
    const int local = base + (rank < extra ? 1 : 0);
    const int first = rank * base + (rank < extra ? rank : extra);

    std::vector<double> hInit(local + 2, 0.0);
    for (int i = 1; i <= local; ++i) {
        const int g = first + i - 1;
        hInit[i] = (g >= 400 && g < 600) ? 100.0 : 0.0;
    }
    const size_t bytes = (local + 2) * sizeof(double);
    double* dU = nullptr;
    double* dNext = nullptr;
    cudaCheck(cudaMalloc(&dU, bytes), "cudaMalloc dU");
    cudaCheck(cudaMalloc(&dNext, bytes), "cudaMalloc dNext");
    cudaCheck(cudaMemcpy(dU, hInit.data(), bytes, cudaMemcpyHostToDevice), "copy init");
    cudaCheck(cudaMemcpy(dNext, hInit.data(), bytes, cudaMemcpyHostToDevice), "copy init");

    // Pinned host staging buffers: [0] = my first cell, [1] = my last cell (outgoing);
    // [2] = left halo, [3] = right halo (incoming).
    double* hStage = nullptr;
    cudaCheck(cudaMallocHost(&hStage, 4 * sizeof(double)), "cudaMallocHost");

    const int left = (rank == 0) ? MPI_PROC_NULL : rank - 1;
    const int right = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;
    const double zero = 0.0;
    const int threads = 256;
    const int blocks = (local + threads - 1) / threads;

    for (int step = 0; step < kSteps; ++step) {
        if (aware) {
            mpiCheck(MPI_Sendrecv(dU + 1, 1, MPI_DOUBLE, left, 0, dU + local + 1, 1, MPI_DOUBLE,
                                  right, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE), "Sendrecv L");
            mpiCheck(MPI_Sendrecv(dU + local, 1, MPI_DOUBLE, right, 1, dU, 1, MPI_DOUBLE, left, 1,
                                  MPI_COMM_WORLD, MPI_STATUS_IGNORE), "Sendrecv R");
        } else {
            cudaCheck(cudaMemcpy(&hStage[0], dU + 1, sizeof(double), cudaMemcpyDeviceToHost), "D2H");
            cudaCheck(cudaMemcpy(&hStage[1], dU + local, sizeof(double), cudaMemcpyDeviceToHost),
                      "D2H");
            hStage[2] = 0.0;
            hStage[3] = 0.0;
            mpiCheck(MPI_Sendrecv(&hStage[0], 1, MPI_DOUBLE, left, 0, &hStage[3], 1, MPI_DOUBLE,
                                  right, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE), "Sendrecv L");
            mpiCheck(MPI_Sendrecv(&hStage[1], 1, MPI_DOUBLE, right, 1, &hStage[2], 1, MPI_DOUBLE,
                                  left, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE), "Sendrecv R");
            cudaCheck(cudaMemcpy(dU, &hStage[2], sizeof(double), cudaMemcpyHostToDevice), "H2D");
            cudaCheck(cudaMemcpy(dU + local + 1, &hStage[3], sizeof(double),
                                 cudaMemcpyHostToDevice), "H2D");
        }
        if (rank == 0) {
            cudaCheck(cudaMemcpy(dU, &zero, sizeof(double), cudaMemcpyHostToDevice), "edge");
        }
        if (rank == size - 1) {
            cudaCheck(cudaMemcpy(dU + local + 1, &zero, sizeof(double), cudaMemcpyHostToDevice),
                      "edge");
        }
        heatStep<<<blocks, threads>>>(dU, dNext, local);
        cudaCheck(cudaGetLastError(), "heatStep launch");
        // The next exchange reads dU on the host side of MPI: the kernel must be finished.
        cudaCheck(cudaDeviceSynchronize(), "heatStep");
        double* t = dU;
        dU = dNext;
        dNext = t;
    }

    std::vector<double> hU(local + 2);
    cudaCheck(cudaMemcpy(hU.data(), dU, bytes, cudaMemcpyDeviceToHost), "D2H result");
    std::vector<int> counts(size);
    std::vector<int> displs(size);
    for (int r = 0; r < size; ++r) {
        counts[r] = base + (r < extra ? 1 : 0);
        displs[r] = r * base + (r < extra ? r : extra);
    }
    std::vector<double> global(rank == 0 ? kCells : 0);
    mpiCheck(MPI_Gatherv(&hU[1], local, MPI_DOUBLE, global.data(), counts.data(), displs.data(),
                         MPI_DOUBLE, 0, MPI_COMM_WORLD), "Gatherv");
    if (rank == 0) {
        std::printf("ranks %d: u[400] = %.12f, u[500] = %.12f, fnv1a = %016llx\n", size,
                    global[400], global[500], static_cast<unsigned long long>(fnv1a(global)));
    }
    cudaCheck(cudaFreeHost(hStage), "cudaFreeHost");
    cudaCheck(cudaFree(dU), "cudaFree");
    cudaCheck(cudaFree(dNext), "cudaFree");
    MPI_Finalize();
    return 0;
}

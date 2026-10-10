// F8-11 Listing 2: the GPU version of the overlapped schedule, for an MPI library that is
// NOT GPU-aware (like this build's, F8-08): CUDA source, compiled with "nvcc -x cu" plus
// the MPI flags. One GPU per rank.
//   compute stream: backward kernels, last layer first; an event marks each bucket ready;
//   copy stream:    waits for the event, copies the bucket to pinned host memory;
//   host thread:    when a copy has finished, starts MPI_Iallreduce on the host bucket and
//                   keeps calling MPI_Testsome so the library makes progress; when a
//                   reduction has finished, copies the result back on the copy stream.
#include <mpi.h>
#include <cuda_runtime.h>

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

constexpr int kLayers = 8;
constexpr int kBucket = 1 << 19;   // floats per bucket

void cudaCheck(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "CUDA error in %s: %s (%s)\n", what, cudaGetErrorName(err),
                     cudaGetErrorString(err));
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

// Stand-in for one layer's backward: small integers, so sums are exact.
__global__ void backwardLayer(float* grad, int n, int layer, int rank)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        grad[i] = static_cast<float>((i + layer + rank) % 17);
    }
}

}  // namespace

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int devices = 0;
    cudaCheck(cudaGetDeviceCount(&devices), "cudaGetDeviceCount");
    cudaCheck(cudaSetDevice(rank % devices), "cudaSetDevice");

    cudaStream_t compute = nullptr;
    cudaStream_t copy = nullptr;
    cudaCheck(cudaStreamCreateWithFlags(&compute, cudaStreamNonBlocking), "stream compute");
    cudaCheck(cudaStreamCreateWithFlags(&copy, cudaStreamNonBlocking), "stream copy");

    std::vector<float*> dGrad(kLayers, nullptr);
    std::vector<float*> hGrad(kLayers, nullptr);
    std::vector<cudaEvent_t> ready(kLayers);
    std::vector<cudaEvent_t> copied(kLayers);
    for (int k = 0; k < kLayers; ++k) {
        cudaCheck(cudaMalloc(&dGrad[k], kBucket * sizeof(float)), "cudaMalloc");
        cudaCheck(cudaMallocHost(&hGrad[k], kBucket * sizeof(float)), "cudaMallocHost");
        cudaCheck(cudaEventCreateWithFlags(&ready[k], cudaEventDisableTiming), "event");
        cudaCheck(cudaEventCreateWithFlags(&copied[k], cudaEventDisableTiming), "event");
    }

    // 1. Enqueue the whole backward pass and the device-to-host copies; nothing waits yet.
    const int threads = 256;
    const int blocks = (kBucket + threads - 1) / threads;
    for (int k = kLayers - 1; k >= 0; --k) {
        backwardLayer<<<blocks, threads, 0, compute>>>(dGrad[k], kBucket, k, rank);
        cudaCheck(cudaGetLastError(), "backwardLayer launch");
        cudaCheck(cudaEventRecord(ready[k], compute), "record ready");
        cudaCheck(cudaStreamWaitEvent(copy, ready[k], 0), "copy waits for ready");
        cudaCheck(cudaMemcpyAsync(hGrad[k], dGrad[k], kBucket * sizeof(float),
                                  cudaMemcpyDeviceToHost, copy), "D2H");
        cudaCheck(cudaEventRecord(copied[k], copy), "record copied");
    }

    // 2. Host loop: start each all-reduce as soon as its bucket is on the host, in the same
    //    order on every rank (collectives must be called in the same order everywhere).
    std::vector<MPI_Request> req(kLayers, MPI_REQUEST_NULL);
    std::vector<bool> started(kLayers, false);
    std::vector<bool> returned(kLayers, false);
    int next = kLayers - 1;   // next bucket to start: buckets become ready from last to first
    int finished = 0;
    while (finished < kLayers) {
        if (next >= 0) {
            const cudaError_t q = cudaEventQuery(copied[next]);
            if (q == cudaSuccess) {
                MPI_Iallreduce(MPI_IN_PLACE, hGrad[next], kBucket, MPI_FLOAT, MPI_SUM,
                               MPI_COMM_WORLD, &req[next]);
                started[next] = true;
                --next;
            } else if (q != cudaErrorNotReady) {
                cudaCheck(q, "cudaEventQuery");
            }
        }
        int outcount = 0;
        std::vector<int> idx(kLayers);
        MPI_Testsome(kLayers, req.data(), &outcount, idx.data(), MPI_STATUSES_IGNORE);
        for (int j = 0; j < outcount && outcount != MPI_UNDEFINED; ++j) {
            const int k = idx[j];
            if (started[k] && !returned[k]) {
                cudaCheck(cudaMemcpyAsync(dGrad[k], hGrad[k], kBucket * sizeof(float),
                                          cudaMemcpyHostToDevice, copy), "H2D");
                returned[k] = true;
                ++finished;
            }
        }
    }
    cudaCheck(cudaStreamSynchronize(copy), "sync copy");

    // 3. Check one value: element 5 of bucket 3 is the sum over ranks of (5 + 3 + r) % 17.
    float v = 0.0f;
    cudaCheck(cudaMemcpy(&v, dGrad[3] + 5, sizeof(float), cudaMemcpyDeviceToHost), "check");
    float expect = 0.0f;
    for (int r = 0; r < size; ++r) {
        expect += static_cast<float>((5 + 3 + r) % 17);
    }
    if (rank == 0) {
        std::printf("bucket 3, element 5: %.1f (expected %.1f) %s\n", v, expect,
                    v == expect ? "PASS" : "FAIL");
    }
    for (int k = 0; k < kLayers; ++k) {
        cudaCheck(cudaFree(dGrad[k]), "cudaFree");
        cudaCheck(cudaFreeHost(hGrad[k]), "cudaFreeHost");
        cudaCheck(cudaEventDestroy(ready[k]), "event destroy");
        cudaCheck(cudaEventDestroy(copied[k]), "event destroy");
    }
    cudaCheck(cudaStreamDestroy(compute), "stream destroy");
    cudaCheck(cudaStreamDestroy(copy), "stream destroy");
    MPI_Finalize();
    return 0;
}

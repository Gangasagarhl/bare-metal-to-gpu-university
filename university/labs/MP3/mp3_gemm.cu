// MP3 Listing 5: the GPU side of the M1 harness. It labels every record with the GPU, driver
// and toolkit, runs the correctness suite's shapes on the real kernels, then times own and
// reference SGEMM and writes RAW records (every timed run) in the format mp3_report reads:
//   ./mp3_gemm > raw.txt ; cat targets.txt raw.txt | ./mp3_report
// Untested on hardware: the build container has no GPU. With the reference column:
//   nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 -DUSE_CUBLAS mp3_gemm.cu -lcublas -o mp3_gemm
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <random>
#include <vector>
#include <cuda_runtime.h>
#ifdef USE_CUBLAS
#include <cublas_v2.h>
#endif
#include "../F6-21/gemm_naive.cuh"
#include "../F6-22/gemm_tiled.cuh"
#define LAUNCH(kernel, grid, block, ...) kernel<<<grid, block>>>(__VA_ARGS__)
#include "../F6-23/gemm_regtile.cuh"
#include "../F6-23/launchers.cuh"
#include "mp3_tolerance.hpp"

using Gemm = std::function<void(const float*, const float*, float*, int, int, int)>;

void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err),
                     cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

struct DeviceBuffer                                       // RAII: freed on every path
{
    float* p = nullptr;
    explicit DeviceBuffer(std::size_t n) { check(cudaMalloc(&p, n * sizeof(float)), "cudaMalloc"); }
    ~DeviceBuffer() { cudaFree(p); }
    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
};

void printEnv()
{
    int runtime = 0, driver = 0, devices = 0;
    check(cudaRuntimeGetVersion(&runtime), "cudaRuntimeGetVersion");
    check(cudaDriverGetVersion(&driver), "cudaDriverGetVersion");
    std::printf("env toolkit runtime-%d\nenv driver supports-cuda-%d\n", runtime, driver);
    std::fflush(stdout);                                  // keep the labels if the next call fails
    check(cudaGetDeviceCount(&devices), "cudaGetDeviceCount");
    cudaDeviceProp prop;
    check(cudaGetDeviceProperties(&prop, 0), "cudaGetDeviceProperties");
    std::printf("env gpu %s\nenv cc %d.%d\n", prop.name, prop.major, prop.minor);
    std::printf("env clocks record-the-setting-by-hand\n");
}

void fill(std::vector<float>& v, unsigned seed)
{
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (float& x : v) { x = dist(gen); }
}

bool correct(const Gemm& gemm, int M, int N, int K)       // same rule as mp3_suite.cpp
{
    std::vector<float> hA(std::size_t(M) * K), hB(std::size_t(K) * N), hC(std::size_t(M) * N);
    fill(hA, 1u);
    fill(hB, 2u);
    DeviceBuffer dA(hA.size()), dB(hB.size()), dC(hC.size());
    check(cudaMemcpy(dA.p, hA.data(), hA.size() * 4, cudaMemcpyHostToDevice), "copy A");
    check(cudaMemcpy(dB.p, hB.data(), hB.size() * 4, cudaMemcpyHostToDevice), "copy B");
    check(cudaMemset(dC.p, 0xFF, hC.size() * 4), "poison C");
    gemm(dA.p, dB.p, dC.p, M, N, K);
    check(cudaGetLastError(), "launch");
    check(cudaMemcpy(hC.data(), dC.p, hC.size() * 4, cudaMemcpyDeviceToHost), "copy C");
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            double exact = 0.0, sumAbs = 0.0;
            for (int k = 0; k < K; ++k) {
                const double p = double(hA[std::size_t(i) * K + k]) * hB[std::size_t(k) * N + j];
                exact += p;
                sumAbs += p < 0 ? -p : p;
            }
            if (!withinTolerance(hC[std::size_t(i) * N + j], exact, sumAbs, P_FP32, K)) {
                return false;
            }
        }
    }
    return true;
}

void timeRaw(const char* impl, int n, const std::function<void()>& run)
{
    cudaEvent_t start, stop;
    check(cudaEventCreate(&start), "cudaEventCreate");
    check(cudaEventCreate(&stop), "cudaEventCreate");
    std::vector<float> ms;
    for (int rep = 0; rep <= 20; ++rep) {                 // rep 0 is the warm-up
        check(cudaEventRecord(start), "cudaEventRecord");
        run();
        check(cudaEventRecord(stop), "cudaEventRecord");
        check(cudaEventSynchronize(stop), "cudaEventSynchronize");
        float t = 0.0f;
        check(cudaEventElapsedTime(&t, start, stop), "cudaEventElapsedTime");
        ms.push_back(t);
    }
    std::printf("runs sgemm %s %d fp32 events warmup %.4f times", impl, n, ms[0]);
    for (int rep = 1; rep <= 20; ++rep) { std::printf(" %.4f", ms[rep]); }
    std::printf("\n");
    check(cudaEventDestroy(start), "cudaEventDestroy");
    check(cudaEventDestroy(stop), "cudaEventDestroy");
}

int main()
{
    printEnv();
    const Gemm own = launchRegTile<128, 128, 8, 8, 8, true>;      // the best CU302 step so far
    bool ok = true;
    const int shapes[][3] = {{1, 1, 1}, {17, 13, 9}, {127, 129, 65}, {1000, 7, 513}, {5, 300, 40}};
    for (const auto& s : shapes) {                       // M1: the shape matrix, now on the GPU
        ok = correct(own, s[0], s[1], s[2]) && ok;
    }
    for (int n : {1024, 2048, 4096}) {
        std::printf("check sgemm %d fp32 %s\n", n, ok ? "pass" : "fail");
        std::vector<float> hA(std::size_t(n) * n), hB(hA.size());
        fill(hA, 3u);
        fill(hB, 4u);
        DeviceBuffer dA(hA.size()), dB(hB.size()), dC(hA.size());
        check(cudaMemcpy(dA.p, hA.data(), hA.size() * 4, cudaMemcpyHostToDevice), "copy A");
        check(cudaMemcpy(dB.p, hB.data(), hB.size() * 4, cudaMemcpyHostToDevice), "copy B");
        timeRaw("own", n, [&] { own(dA.p, dB.p, dC.p, n, n, n); });
        check(cudaGetLastError(), "launch");
#ifdef USE_CUBLAS
        cublasHandle_t h;
        if (cublasCreate(&h) != CUBLAS_STATUS_SUCCESS) { std::exit(EXIT_FAILURE); }
        const float one = 1.0f, zero = 0.0f;                     // row-major trick of F6-21
        timeRaw("ref", n, [&] {
            if (cublasSgemm(h, CUBLAS_OP_N, CUBLAS_OP_N, n, n, n, &one, dB.p, n, dA.p, n, &zero,
                            dC.p, n) != CUBLAS_STATUS_SUCCESS) { std::exit(EXIT_FAILURE); }
        });
        cublasDestroy(h);
#endif
    }
    return ok ? 0 : 1;
}

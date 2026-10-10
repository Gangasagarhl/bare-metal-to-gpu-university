// gemm_harness.cuh: the E6/E7 test-and-time harness shared by the CU302 SGEMM labs.
// For every kernel variant: (1) correctness on awkward shapes against a CPU FP64
// reference, (2) registers, shared memory and occupancy from the runtime, (3) median
// time of 20 runs after a warm-up at three sizes, against cuBLAS SGEMM on the same
// data, with the result checked against cuBLAS. Untested on hardware (no GPU here).
// cuBLAS is used only when built with -DUSE_CUBLAS and linked with -lcublas (run.sh does);
// without it the table has no cuBLAS column and results are checked on the shapes only.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>
#include <cuda_runtime.h>
#ifdef USE_CUBLAS
#include <cublas_v2.h>
#endif

struct Variant
{
    const char* name;
    const void* func;                                    // the kernel, for attribute queries
    int threadsPerBlock;
    void (*launch)(const float* dA, const float* dB, float* dC, int M, int N, int K);
};

inline void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err),
                     cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

// Error bound for a K-term FP32 dot product in any order (gamma_K * sum |a||b|),
// with u = 2^-24, plus the same again for the comparison partner when it is FP32 too.
inline bool withinBound(double got, double ref, double sumAbs, int K, double partners)
{
    const double u = std::ldexp(1.0, -24);
    const double gamma = K * u / (1.0 - K * u);
    return std::fabs(got - ref) <= partners * gamma * sumAbs + 1e-30;
}

inline void fill(std::vector<float>& v, unsigned seed)
{
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (float& x : v) {
        x = dist(gen);
    }
}

#ifdef USE_CUBLAS
inline void checkBlas(cublasStatus_t st, const char* what)
{
    if (st != CUBLAS_STATUS_SUCCESS) {
        std::fprintf(stderr, "%s failed: cuBLAS status %d\n", what, static_cast<int>(st));
        std::exit(EXIT_FAILURE);
    }
}

// C = A * B with cuBLAS. cuBLAS is column-major: a row-major M x N matrix is its
// N x M transpose, so we ask for C^T = B^T * A^T, which needs no copies.
inline void blasSgemm(cublasHandle_t h, const float* dA, const float* dB, float* dC, int M, int N,
                      int K)
{
    const float alpha = 1.0f, beta = 0.0f;
    checkBlas(cublasSgemm(h, CUBLAS_OP_N, CUBLAS_OP_N, N, M, K, &alpha, dB, N, dA, K, &beta, dC, N),
              "cublasSgemm");
}
#endif

inline bool testShape(const Variant& v, int M, int N, int K)
{
    std::vector<float> hA(size_t(M) * K), hB(size_t(K) * N), hC(size_t(M) * N);
    fill(hA, 1u);
    fill(hB, 2u);
    float *dA, *dB, *dC;
    check(cudaMalloc(&dA, hA.size() * sizeof(float)), "cudaMalloc A");
    check(cudaMalloc(&dB, hB.size() * sizeof(float)), "cudaMalloc B");
    check(cudaMalloc(&dC, hC.size() * sizeof(float)), "cudaMalloc C");
    check(cudaMemcpy(dA, hA.data(), hA.size() * sizeof(float), cudaMemcpyHostToDevice), "copy A");
    check(cudaMemcpy(dB, hB.data(), hB.size() * sizeof(float), cudaMemcpyHostToDevice), "copy B");
    check(cudaMemset(dC, 0xFF, hC.size() * sizeof(float)), "poison C");   // NaN everywhere
    v.launch(dA, dB, dC, M, N, K);
    check(cudaGetLastError(), "kernel launch");
    check(cudaMemcpy(hC.data(), dC, hC.size() * sizeof(float), cudaMemcpyDeviceToHost), "copy C");
    int bad = 0;
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            double ref = 0.0, sumAbs = 0.0;
            for (int k = 0; k < K; ++k) {
                const double p = double(hA[size_t(i) * K + k]) * hB[size_t(k) * N + j];
                ref += p;
                sumAbs += std::fabs(p);
            }
            if (!withinBound(hC[size_t(i) * N + j], ref, sumAbs, K, 1.0)) {
                ++bad;
            }
        }
    }
    check(cudaFree(dA), "cudaFree");
    check(cudaFree(dB), "cudaFree");
    check(cudaFree(dC), "cudaFree");
    std::printf("  %-14s M=%5d N=%5d K=%5d : %s (%d elements outside the bound)\n", v.name, M, N,
                K, bad ? "FAIL" : "ok", bad);
    return bad == 0;
}

template <typename F>
float medianMs(F&& run)
{
    cudaEvent_t start, stop;
    check(cudaEventCreate(&start), "cudaEventCreate");
    check(cudaEventCreate(&stop), "cudaEventCreate");
    run();                                                // warm-up
    check(cudaDeviceSynchronize(), "warm-up");
    std::vector<float> ms;
    for (int rep = 0; rep < 20; ++rep) {
        check(cudaEventRecord(start), "cudaEventRecord");
        run();
        check(cudaEventRecord(stop), "cudaEventRecord");
        check(cudaEventSynchronize(stop), "cudaEventSynchronize");
        float t = 0.0f;
        check(cudaEventElapsedTime(&t, start, stop), "cudaEventElapsedTime");
        ms.push_back(t);
    }
    check(cudaEventDestroy(start), "cudaEventDestroy");
    check(cudaEventDestroy(stop), "cudaEventDestroy");
    std::sort(ms.begin(), ms.end());
    return ms[ms.size() / 2];
}

inline int runE6(const std::vector<Variant>& variants)
{
    int devices = 0;
    check(cudaGetDeviceCount(&devices), "cudaGetDeviceCount");
    cudaDeviceProp prop;
    check(cudaGetDeviceProperties(&prop, 0), "cudaGetDeviceProperties");
    std::printf("GPU: %s, compute capability %d.%d\n", prop.name, prop.major, prop.minor);

    bool allOk = true;
    std::printf("correctness (bound: gamma_K * sum|a*b|, FP64 reference):\n");
    const int shapes[][3] = {{1, 1, 1}, {17, 13, 9}, {64, 64, 64}, {127, 129, 65},
                             {256, 32, 300}, {33, 512, 64}, {1000, 7, 513}, {128, 128, 1}};
    for (const Variant& v : variants) {
        for (const auto& s : shapes) {
            allOk = testShape(v, s[0], s[1], s[2]) && allOk;
        }
    }

    std::printf("resources:\n");
    for (const Variant& v : variants) {
        cudaFuncAttributes attr;
        check(cudaFuncGetAttributes(&attr, v.func), "cudaFuncGetAttributes");
        int blocks = 0;
        check(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&blocks, v.func, v.threadsPerBlock, 0),
              "cudaOccupancyMaxActiveBlocksPerMultiprocessor");
        std::printf("  %-14s %3d registers, %6zu B shared, %4d threads/block, occupancy %5.1f %%\n",
                    v.name, attr.numRegs, attr.sharedSizeBytes, v.threadsPerBlock,
                    100.0 * blocks * v.threadsPerBlock / prop.maxThreadsPerMultiProcessor);
    }

#ifdef USE_CUBLAS
    cublasHandle_t h;
    checkBlas(cublasCreate(&h), "cublasCreate");
    checkBlas(cublasSetMathMode(h, CUBLAS_DEFAULT_MATH), "cublasSetMathMode");
#else
    std::printf("(built without -DUSE_CUBLAS: no cuBLAS column, no comparison at large sizes)\n");
#endif
    std::printf("speed (median of 20 runs after one warm-up):\n");
    for (int n : {1024, 2048, 4096}) {
        const int M = n, N = n, K = n;
        std::vector<float> hA(size_t(M) * K), hB(size_t(K) * N), hC(size_t(M) * N),
            hRef(size_t(M) * N);
        fill(hA, 3u);
        fill(hB, 4u);
        float *dA, *dB, *dC, *dRef;
        check(cudaMalloc(&dA, hA.size() * sizeof(float)), "cudaMalloc A");
        check(cudaMalloc(&dB, hB.size() * sizeof(float)), "cudaMalloc B");
        check(cudaMalloc(&dC, hC.size() * sizeof(float)), "cudaMalloc C");
        check(cudaMalloc(&dRef, hRef.size() * sizeof(float)), "cudaMalloc Ref");
        check(cudaMemcpy(dA, hA.data(), hA.size() * sizeof(float), cudaMemcpyHostToDevice), "copy A");
        check(cudaMemcpy(dB, hB.data(), hB.size() * sizeof(float), cudaMemcpyHostToDevice), "copy B");
        const double flop = 2.0 * M * N * K;
        float tBlas = 0.0f;
#ifdef USE_CUBLAS
        tBlas = medianMs([&] { blasSgemm(h, dA, dB, dRef, M, N, K); });
        check(cudaMemcpy(hRef.data(), dRef, hRef.size() * sizeof(float), cudaMemcpyDeviceToHost),
              "copy Ref");
        std::printf("  %-14s n=%5d %9.3f ms %9.1f GFLOP/s  100.0 %% of cuBLAS\n", "cuBLAS", n,
                    tBlas, flop / (tBlas * 1e-3) / 1e9);
#endif
        for (const Variant& v : variants) {
            const float t = medianMs([&] { v.launch(dA, dB, dC, M, N, K); });
            check(cudaGetLastError(), "kernel launch");
            check(cudaMemcpy(hC.data(), dC, hC.size() * sizeof(float), cudaMemcpyDeviceToHost),
                  "copy C");
            int bad = 0;
            if (tBlas > 0.0f) {       // |a*b| <= 1 here, so sum|a*b| <= K bounds both errors
                for (size_t i = 0; i < hC.size(); ++i) {
                    bad += withinBound(hC[i], hRef[i], K, K, 2.0) ? 0 : 1;
                }
            }
            allOk = allOk && bad == 0;
            std::printf("  %-14s n=%5d %9.3f ms %9.1f GFLOP/s %6.1f %% of cuBLAS  %s\n", v.name, n,
                        t, flop / (t * 1e-3) / 1e9, tBlas > 0.0f ? 100.0 * tBlas / t : 0.0,
                        tBlas > 0.0f ? (bad ? "MISMATCH" : "matches cuBLAS") : "not compared");
        }
        check(cudaFree(dA), "cudaFree");
        check(cudaFree(dB), "cudaFree");
        check(cudaFree(dC), "cudaFree");
        check(cudaFree(dRef), "cudaFree");
    }
#ifdef USE_CUBLAS
    checkBlas(cublasDestroy(h), "cublasDestroy");
#endif
    std::printf("%s\n", allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");
    return allOk ? 0 : 1;
}

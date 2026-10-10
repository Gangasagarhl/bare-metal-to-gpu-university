// F6-26 Listing 2: E7, mixed-precision GEMM on tensor cores, tested and timed.
// For FP16 and BF16 inputs: correctness on shapes (multiples of 16) against an FP64
// reference, with the tolerance justified in F6-25; then median time at three sizes,
// against cuBLAS (cublasGemmEx, same input type, FP32 output and compute) when built with
// -DUSE_CUBLAS -lcublas. Untested on hardware. Build (as run.sh does):
//   nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 -DUSE_CUBLAS wmma_gemm.cu -lcublas
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <type_traits>
#include <vector>
#include <cuda_runtime.h>
#ifdef USE_CUBLAS
#include <cublas_v2.h>
#endif
#include "gemm_wmma.cuh"

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err),
                     cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

template <typename T> float toFloat(T x);
template <> float toFloat<__half>(__half x) { return __half2float(x); }
template <> float toFloat<__nv_bfloat16>(__nv_bfloat16 x) { return __bfloat162float(x); }
template <typename T> T fromFloat(float x);
template <> __half fromFloat<__half>(float x) { return __float2half_rn(x); }
template <> __nv_bfloat16 fromFloat<__nv_bfloat16>(float x) { return __float2bfloat16_rn(x); }
template <typename T> double unitRoundoff();
template <> double unitRoundoff<__half>() { return std::ldexp(1.0, -11); }
template <> double unitRoundoff<__nv_bfloat16>() { return std::ldexp(1.0, -8); }

template <typename T>
bool launchWmma(const T* dA, const T* dB, float* dC, int M, int N, int K)
{
    if (M % 16 != 0 || N % 16 != 0 || K % 16 != 0) {
        return false;                               // pad the matrices first (an exercise)
    }
    const dim3 grid((N + BLOCK_TILE - 1) / BLOCK_TILE, (M + BLOCK_TILE - 1) / BLOCK_TILE);
    gemmWmma<T><<<grid, 128>>>(dA, dB, dC, M, N, K);
    return true;
}

template <typename T>
bool testShape(const char* name, int M, int N, int K)
{
    std::mt19937 gen(3u);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> a32(size_t(M) * K), b32(size_t(K) * N), c(size_t(M) * N);
    std::vector<T> a(a32.size()), b(b32.size());
    for (size_t i = 0; i < a32.size(); ++i) { a32[i] = dist(gen); a[i] = fromFloat<T>(a32[i]); }
    for (size_t i = 0; i < b32.size(); ++i) { b32[i] = dist(gen); b[i] = fromFloat<T>(b32[i]); }
    T *dA, *dB;
    float* dC;
    check(cudaMalloc(&dA, a.size() * sizeof(T)), "cudaMalloc A");
    check(cudaMalloc(&dB, b.size() * sizeof(T)), "cudaMalloc B");
    check(cudaMalloc(&dC, c.size() * sizeof(float)), "cudaMalloc C");
    check(cudaMemcpy(dA, a.data(), a.size() * sizeof(T), cudaMemcpyHostToDevice), "copy A");
    check(cudaMemcpy(dB, b.data(), b.size() * sizeof(T), cudaMemcpyHostToDevice), "copy B");
    check(cudaMemset(dC, 0xFF, c.size() * sizeof(float)), "poison C");
    launchWmma(dA, dB, dC, M, N, K);
    check(cudaGetLastError(), "launch gemmWmma");
    check(cudaMemcpy(c.data(), dC, c.size() * sizeof(float), cudaMemcpyDeviceToHost), "copy C");
    // tolerance from F6-25: input rounding (2u_in + u_in^2)(1 + gamma_K) plus FP32
    // accumulation gamma_K, times sum|a*b| of the original FP32 values
    const double uin = unitRoundoff<T>(), u32 = std::ldexp(1.0, -24);
    const double g = K * u32 / (1 - K * u32);
    const double factor = (2 * uin + uin * uin) * (1 + g) + g;
    int bad = 0;
    double worst = 0.0;
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            double ref = 0.0, sumAbs = 0.0;
            for (int k = 0; k < K; ++k) {
                const double p = double(a32[size_t(i) * K + k]) * b32[size_t(k) * N + j];
                ref += p;
                sumAbs += std::fabs(p);
            }
            const double ratio = std::fabs(c[size_t(i) * N + j] - ref) / (factor * sumAbs);
            if (!(ratio <= 1.0)) {
                ++bad;
            } else {
                worst = std::max(worst, ratio);
            }
        }
    }
    check(cudaFree(dA), "cudaFree");
    check(cudaFree(dB), "cudaFree");
    check(cudaFree(dC), "cudaFree");
    std::printf("  %-5s M=%5d N=%5d K=%5d : %s (%d outside the tolerance; worst %.3f of it)\n", name,
                M, N, K, bad ? "FAIL" : "ok", bad, worst);
    return bad == 0;
}

template <typename T>
float medianMs(const T* dA, const T* dB, float* dC, int n, bool library)
{
    cudaEvent_t start, stop;
    check(cudaEventCreate(&start), "cudaEventCreate");
    check(cudaEventCreate(&stop), "cudaEventCreate");
#ifdef USE_CUBLAS
    static cublasHandle_t h = nullptr;
    if (library && h == nullptr && cublasCreate(&h) != CUBLAS_STATUS_SUCCESS) {
        std::fprintf(stderr, "cublasCreate failed\n");
        std::exit(EXIT_FAILURE);
    }
    const cudaDataType_t type = std::is_same<T, __half>::value ? CUDA_R_16F
                                                                                 : CUDA_R_16BF;
#endif
    auto run = [&] {
        if (!library) {
            launchWmma(dA, dB, dC, n, n, n);
            return;
        }
#ifdef USE_CUBLAS
        const float alpha = 1.0f, beta = 0.0f;   // row-major trick of F6-21: C^T = B^T A^T
        if (cublasGemmEx(h, CUBLAS_OP_N, CUBLAS_OP_N, n, n, n, &alpha, dB, type, n, dA, type, n,
                         &beta, dC, CUDA_R_32F, n, CUBLAS_COMPUTE_32F,
                         CUBLAS_GEMM_DEFAULT) != CUBLAS_STATUS_SUCCESS) {
            std::fprintf(stderr, "cublasGemmEx failed\n");
            std::exit(EXIT_FAILURE);
        }
#endif
    };
    run();
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

template <typename T>
bool runType(const char* name)
{
    bool ok = true;
    const int shapes[][3] = {{16, 16, 16}, {48, 80, 32}, {64, 64, 64}, {112, 208, 96},
                             {256, 16, 512}};
    for (const auto& s : shapes) {
        ok = testShape<T>(name, s[0], s[1], s[2]) && ok;
    }
    for (int n : {1024, 2048, 4096}) {
        T *dA, *dB;
        float* dC;
        check(cudaMalloc(&dA, size_t(n) * n * sizeof(T)), "cudaMalloc A");
        check(cudaMalloc(&dB, size_t(n) * n * sizeof(T)), "cudaMalloc B");
        check(cudaMalloc(&dC, size_t(n) * n * sizeof(float)), "cudaMalloc C");
        check(cudaMemset(dA, 0, size_t(n) * n * sizeof(T)), "cudaMemset");
        check(cudaMemset(dB, 0, size_t(n) * n * sizeof(T)), "cudaMemset");
        const double flop = 2.0 * n * n * n;
        const float t = medianMs(dA, dB, dC, n, false);
        std::printf("  %-5s n=%5d WMMA  %9.3f ms %9.1f GFLOP/s\n", name, n, t, flop / (t * 1e-3) / 1e9);
#ifdef USE_CUBLAS
        const float tl = medianMs(dA, dB, dC, n, true);
        std::printf("  %-5s n=%5d cuBLAS %8.3f ms %9.1f GFLOP/s  (WMMA kernel: %.1f %% of cuBLAS)\n",
                    name, n, tl, flop / (tl * 1e-3) / 1e9, 100.0 * tl / t);
#endif
        check(cudaFree(dA), "cudaFree");
        check(cudaFree(dB), "cudaFree");
        check(cudaFree(dC), "cudaFree");
    }
    return ok;
}

int main()
{
    int devices = 0;
    check(cudaGetDeviceCount(&devices), "cudaGetDeviceCount");
    const bool ok = runType<__half>("FP16") & runType<__nv_bfloat16>("BF16");
    std::printf("%s\n", ok ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");
    return ok ? 0 : 1;
}

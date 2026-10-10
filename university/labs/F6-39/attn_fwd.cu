// F6-39 Listing 2: a simple tiled attention forward kernel (one head, FP32), the structure of
// Listing 1's tiled() on a GPU. One thread block per tile of BR query rows, one thread per query
// row; K and V tiles of BC rows are staged in shared memory and shared by the whole block.
// A teaching kernel: correct structure, not tuned. Untested on hardware (no GPU in the build).
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

constexpr int D = 64;    // head dimension, fixed at compile time so q and acc live in registers
constexpr int BR = 32;   // query rows per block (= threads per block)
constexpr int BC = 32;   // key/value rows per shared-memory tile

__global__ void attnForward(const float* q, const float* k, const float* v, float* o, int n, float scale)
{
    __shared__ float kTile[BC][D];
    __shared__ float vTile[BC][D];
    const int i = blockIdx.x * BR + threadIdx.x;       // this thread's query row
    const bool active = i < n;
    float qr[D];
    float acc[D];
    for (int x = 0; x < D; ++x) {
        qr[x] = active ? q[static_cast<long long>(i) * D + x] * scale : 0.0f;
        acc[x] = 0.0f;
    }
    float m = -INFINITY;
    float l = 0.0f;
    for (int j0 = 0; j0 < n; j0 += BC) {
        for (int e = threadIdx.x; e < BC * D; e += BR) {   // the block loads the tile together
            const int r = e / D;
            const int x = e % D;
            const bool in = j0 + r < n;
            kTile[r][x] = in ? k[static_cast<long long>(j0 + r) * D + x] : 0.0f;
            vTile[r][x] = in ? v[static_cast<long long>(j0 + r) * D + x] : 0.0f;
        }
        __syncthreads();                                   // tile complete before anyone reads it
        const int cols = min(BC, n - j0);
        float s[BC];
        float tileMax = -INFINITY;
        for (int c = 0; c < cols; ++c) {
            float dot = 0.0f;
            for (int x = 0; x < D; ++x) {
                dot += qr[x] * kTile[c][x];
            }
            s[c] = dot;
            tileMax = fmaxf(tileMax, dot);
        }
        const float mNew = fmaxf(m, tileMax);
        const float alpha = __expf(m - mNew);              // 0 on the first tile (m = -inf)
        float rowSum = 0.0f;
        for (int x = 0; x < D; ++x) {
            acc[x] *= alpha;
        }
        for (int c = 0; c < cols; ++c) {
            const float p = __expf(s[c] - mNew);
            rowSum += p;
            for (int x = 0; x < D; ++x) {
                acc[x] += p * vTile[c][x];
            }
        }
        l = l * alpha + rowSum;
        m = mNew;
        __syncthreads();                                   // everyone done before the next load
    }
    if (active) {
        for (int x = 0; x < D; ++x) {
            o[static_cast<long long>(i) * D + x] = acc[x] / l;
        }
    }
}

int main()
{
    const int n = 1000;                                    // not a multiple of BR or BC
    const std::size_t count = static_cast<std::size_t>(n) * D;
    std::vector<float> hQ(count), hK(count), hV(count);
    for (std::size_t i = 0; i < count; ++i) {
        hQ[i] = static_cast<float>(static_cast<int>((i * 37) % 23) - 11) * 0.15f;
        hK[i] = static_cast<float>(static_cast<int>((i * 53) % 19) - 9) * 0.2f;
        hV[i] = static_cast<float>(static_cast<int>((i * 29) % 17) - 8) * 0.1f;
    }
    float* dQ = nullptr;
    float* dK = nullptr;
    float* dV = nullptr;
    float* dO = nullptr;
    check(cudaMalloc(&dQ, count * sizeof(float)), "cudaMalloc Q");
    check(cudaMalloc(&dK, count * sizeof(float)), "cudaMalloc K");
    check(cudaMalloc(&dV, count * sizeof(float)), "cudaMalloc V");
    check(cudaMalloc(&dO, count * sizeof(float)), "cudaMalloc O");
    check(cudaMemcpy(dQ, hQ.data(), count * sizeof(float), cudaMemcpyHostToDevice), "copy Q");
    check(cudaMemcpy(dK, hK.data(), count * sizeof(float), cudaMemcpyHostToDevice), "copy K");
    check(cudaMemcpy(dV, hV.data(), count * sizeof(float), cudaMemcpyHostToDevice), "copy V");
    const int blocks = (n + BR - 1) / BR;
    attnForward<<<blocks, BR>>>(dQ, dK, dV, dO, n, 1.0f / std::sqrt(static_cast<float>(D)));
    check(cudaGetLastError(), "attnForward launch");
    std::vector<float> hO(count);
    check(cudaMemcpy(hO.data(), dO, count * sizeof(float), cudaMemcpyDeviceToHost), "copy O");
    // CPU reference for a few rows, in double
    double worst = 0.0;
    for (int i = 0; i < n; i += 97) {
        std::vector<double> s(n);
        double m = -1e300;
        for (int j = 0; j < n; ++j) {
            double dot = 0.0;
            for (int x = 0; x < D; ++x) {
                dot += static_cast<double>(hQ[i * D + x]) * hK[j * D + x];
            }
            s[j] = dot / std::sqrt(static_cast<double>(D));
            m = std::fmax(m, s[j]);
        }
        double l = 0.0;
        for (int j = 0; j < n; ++j) {
            s[j] = std::exp(s[j] - m);
            l += s[j];
        }
        for (int x = 0; x < D; ++x) {
            double ref = 0.0;
            for (int j = 0; j < n; ++j) {
                ref += s[j] * hV[j * D + x];
            }
            worst = std::fmax(worst, std::fabs(ref / l - hO[i * D + x]));
        }
    }
    std::printf("attention forward: N %d, d %d, worst |O - reference| on sampled rows = %g\n", n, D, worst);
    check(cudaFree(dQ), "cudaFree Q");
    check(cudaFree(dK), "cudaFree K");
    check(cudaFree(dV), "cudaFree V");
    check(cudaFree(dO), "cudaFree O");
    return 0;
}

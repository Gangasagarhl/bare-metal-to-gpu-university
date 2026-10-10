// MP3 Listing 2: the milestone-1 correctness suite, run on the CPU emulator of F6-21.
// It runs the learner's own CU302 kernels UNCHANGED (F6-21 naive, F6-22 tiled, F6-23
// register-tiled) over the MP3 shape matrix, in FP32 and with FP16/BF16-rounded inputs,
// against an FP64 reference with the tolerance of mp3_tolerance.hpp. It then runs three
// seeded faults and demands that each one is caught. Correctness only: no speed here.
#include "../F6-21/cuda_shim.hpp"
#define LAUNCH(kernel, grid, block, ...) launch(grid, block, kernel, __VA_ARGS__)
#include "../F6-21/gemm_naive.cuh"
#include "../F6-22/gemm_tiled.cuh"
#include "../F6-23/gemm_regtile.cuh"
#include "../F6-23/launchers.cuh"
#include "mp3_tolerance.hpp"
#include "mp3_mutants.cuh"
#include <cmath>
#include <cstdio>
#include <functional>
#include <random>
#include <string>
#include <vector>

using Gemm = std::function<void(const float*, const float*, float*, int, int, int)>;

struct Shape
{
    int M, N, K;
};

// The shape matrix: each row is there to catch one class of bug (see the handbook, M1).
#ifdef MP3_SQUARE_ONLY      // the weak suite the handbook warns about: square tile multiples only
const Shape kShapes[] = {{32, 32, 32}, {64, 64, 64}};
#else
const Shape kShapes[] = {{1, 1, 1},   {32, 32, 32}, {48, 48, 1},  {17, 13, 9},
                         {33, 31, 65}, {70, 5, 33},  {5, 70, 40},  {24, 36, 36}};
#endif

struct Result
{
    int passed = 0;
    int total = 0;
    double worst = 0.0;          // largest error / bound over all passing elements
    std::string firstFail;
};

Result runCase(const Gemm& gemm, const Precision& p)
{
    Result r;
    for (const Shape& s : kShapes) {
        const std::size_t nA = std::size_t(s.M) * s.K, nB = std::size_t(s.K) * s.N;
        std::vector<float> A(nA), B(nB), C(std::size_t(s.M) * s.N, std::nanf(""));
        std::mt19937 gen(unsigned(s.M * 7919 + s.N * 104729 + s.K));
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (float& x : A) { x = dist(gen); }
        for (float& x : B) { x = dist(gen); }
        std::vector<float> Ain(A), Bin(B);             // what the kernel receives
        if (p.roundsInputs) {
            for (float& x : Ain) { x = static_cast<float>(roundTo(x, p.input)); }
            for (float& x : Bin) { x = static_cast<float>(roundTo(x, p.input)); }
        }
        gemm(Ain.data(), Bin.data(), C.data(), s.M, s.N, s.K);
        int bad = 0;
        for (int i = 0; i < s.M; ++i) {
            for (int j = 0; j < s.N; ++j) {
                double exact = 0.0, sumAbs = 0.0;      // FP64 dot product of the ORIGINAL inputs
                for (int k = 0; k < s.K; ++k) {
                    const double prod =
                        double(A[std::size_t(i) * s.K + k]) * B[std::size_t(k) * s.N + j];
                    exact += prod;
                    sumAbs += std::fabs(prod);
                }
                const double got = C[std::size_t(i) * s.N + j];
                if (!withinTolerance(got, exact, sumAbs, p, s.K)) {
                    ++bad;
                } else if (sumAbs > 0.0) {
                    const double ratio = std::fabs(got - exact) / (relBound(p, s.K) * sumAbs);
                    r.worst = std::fmax(r.worst, ratio);
                }
            }
        }
        ++r.total;
        if (bad == 0) {
            ++r.passed;
        } else if (r.firstFail.empty()) {
            r.firstFail = "M=" + std::to_string(s.M) + " N=" + std::to_string(s.N) +
                          " K=" + std::to_string(s.K) + ", " + std::to_string(bad) + " bad";
        }
    }
    return r;
}

void launchNaive(const float* A, const float* B, float* C, int M, int N, int K)
{
    launch(dim3((N + 15) / 16, (M + 15) / 16), dim3(16, 16), sgemmNaive, A, B, C, M, N, K);
}

void launchTiled16(const float* A, const float* B, float* C, int M, int N, int K)
{
    launch(dim3((N + 15) / 16, (M + 15) / 16), dim3(16, 16), sgemmTiled<16>, A, B, C, M, N, K);
}

void launchSkipTail(const float* A, const float* B, float* C, int M, int N, int K)
{
    launch(dim3((N + 15) / 16, (M + 15) / 16), dim3(16, 16), mutantSkipTail<16>, A, B, C, M, N, K);
}

void launchFloorGrid(const float* A, const float* B, float* C, int M, int N, int K)
{
    launch(dim3(N / 16, M / 16), dim3(16, 16), sgemmNaive, A, B, C, M, N, K);  // SLIP: rounds down
}

void launchBf16Acc(const float* A, const float* B, float* C, int M, int N, int K)
{
    launch(dim3((N + 15) / 16, (M + 15) / 16), dim3(16, 16), mutantBf16Acc, A, B, C, M, N, K);
}

struct Case
{
    const char* name;
    Gemm gemm;
    const Precision* precision;
    bool seededFault;
};

int main()
{
    const Case cases[] = {
        {"naive (F6-21)", launchNaive, &P_FP32, false},
        {"tiled<16> (F6-22)", launchTiled16, &P_FP32, false},
        {"reg2d 64x64/4x4 (F6-23)", launchRegTile<64, 64, 8, 4, 4, false>, &P_FP32, false},
        {"vec 128x128/8x8 (F6-23)", launchRegTile<128, 128, 8, 8, 8, true>, &P_FP32, false},
        {"tiled<16>, rounded inputs", launchTiled16, &P_FP16, false},
        {"tiled<16>, rounded inputs", launchTiled16, &P_BF16, false},
        {"fault 1: skip partial K tile", launchSkipTail, &P_FP32, true},
        {"fault 2: grid rounded down", launchFloorGrid, &P_FP32, true},
        {"fault 3: BF16 accumulator", launchBf16Acc, &P_BF16, true},
    };
    std::printf("MP3 M1 correctness suite on the CPU emulator (no GPU): %zu shapes, "
                "FP64 reference\n", sizeof(kShapes) / sizeof(kShapes[0]));
    std::printf("relative bound per precision (x sum|a*b|):\n");
    for (const Precision* p : {&P_FP32, &P_FP16, &P_BF16}) {
        std::printf("  %-17s K=1: %.3g   K=64: %.3g   K=4096: %.3g\n", p->name, relBound(*p, 1),
                    relBound(*p, 64), relBound(*p, 4096));
    }
    std::printf("%-30s %-17s %-7s %-10s %s\n", "kernel", "precision", "shapes", "worst", "verdict");
    int correctFailed = 0, faultsMissed = 0, faults = 0;
    for (const Case& c : cases) {
        const Result r = runCase(c.gemm, *c.precision);
        const bool allPass = r.passed == r.total;
        std::string verdict;
        if (!c.seededFault) {
            verdict = allPass ? "pass" : "FAIL (" + r.firstFail + ")";
            correctFailed += allPass ? 0 : 1;
        } else {
            ++faults;
            verdict = allPass ? "MISSED: the suite is too weak"
                              : "caught (first: " + r.firstFail + ")";
            faultsMissed += allPass ? 1 : 0;
        }
        std::printf("%-30s %-17s %d/%-5d %-10.3f %s\n", c.name, c.precision->name, r.passed,
                    r.total, r.worst, verdict.c_str());
    }
    const bool ok = correctFailed == 0 && faultsMissed == 0;
    std::printf("%s: %d correct-kernel case(s) failed, %d of %d seeded faults caught\n",
                ok ? "SUITE OK" : "SUITE NOT OK", correctFailed, faults - faultsMissed, faults);
    return ok ? 0 : 1;
}

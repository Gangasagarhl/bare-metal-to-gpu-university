// MP4 Listing 13: forensic evidence generator: "passes on gfx90a, wrong on gfx1100".
// Runs mutantStride256 (the loader copied from F7-16 with its literal stride) for every
// instance on two shapes in the CPU emulator, with C pre-filled with -7 so unwritten output
// is visible. The output is the forensic lab's evidence pack. Exit 0 = evidence reproduced.
#include "../F6-21/cuda_shim.hpp"
#define LAUNCH(kernel, grid, block, ...) launch(grid, block, kernel, __VA_ARGS__)
#include "mp4_check.hpp"
#include "mp4_mutants.hpp"
#include "shapes.hpp"
#include "tuning.hpp"
#include <cstdio>
#include <vector>

template <class Cfg>
int caseRun(const char* name, int M, int N, int K)
{
    const std::vector<float> A = testMatrix(M, K, 1u);
    const std::vector<float> B = testMatrix(K, N, 2u);
    std::vector<float> C(static_cast<std::size_t>(M) * N, -7.0f);
    const dim3 grid((N + Cfg::BN - 1) / Cfg::BN, (M + Cfg::BM - 1) / Cfg::BM);
    LAUNCH(mutantStride256<Cfg>, grid, dim3(Cfg::THREADS), A.data(), B.data(), C.data(), M, N, K);
    const Verdict v = checkGemm(A, B, C, M, N, K);
    std::printf("%-18s work-items %3d  %3dx%3dx%3d  outside tolerance %5d of %5d  C[0] %+.6f\n", name,
                Cfg::THREADS, M, N, K, v.bad, M * N, C[0]);
    return v.bad;
}

int main()
{
    std::printf("tuning rows:");
    for (const TuningRow& t : kTuning) {
        std::printf("  %s -> %s", t.arch, kInstances[t.instance].name);
    }
    std::printf("\n");
    int badBig = 0;
    int badSmall = 0;
    for (const Shape& s : {Shape{64, 64, 64}, Shape{33, 31, 65}}) {
        badBig += caseRun<Inst0>(kInstances[0].name, s.M, s.N, s.K);
        badBig += caseRun<Inst1>(kInstances[1].name, s.M, s.N, s.K);
        badSmall += caseRun<Inst2>(kInstances[2].name, s.M, s.N, s.K);
    }
    std::printf("256-work-item instances: %d bad elements; 128-work-item instance: %d bad elements\n",
                badBig, badSmall);
    return (badBig == 0 && badSmall > 0) ? 0 : 1;
}

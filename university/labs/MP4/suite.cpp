// MP4 Listing 8: milestone-1 starter suite, run on the CPU (no GPU). It checks:
//   1. the instance table (work-items, LDS bytes, FMAs per LDS read);
//   2. the tuning table (known instance, whole waves, no duplicate device keys) and archKey;
//   3. the GPU kernel itself, unchanged, for every instance on every shape, in the F6-21
//      emulator, against the FP64 reference with MP3's tolerance;
//   4. that the suite can fail: a seeded fault must be caught by at least one shape.
#include "../F6-21/cuda_shim.hpp"
#define LAUNCH(kernel, grid, block, ...) launch(grid, block, kernel, __VA_ARGS__)
#include "launchers.hpp"
#include "mp4_check.hpp"
#include "mp4_mutants.hpp"
#include "shapes.hpp"
#include <cstdio>
#include <string>
#include <vector>

using Runner = void (*)(const float*, const float*, float*, int, int, int);

template <class Cfg>
void runClampK(const float* A, const float* B, float* C, int M, int N, int K)
{
    const dim3 grid((N + Cfg::BN - 1) / Cfg::BN, (M + Cfg::BM - 1) / Cfg::BM);
    LAUNCH(mutantClampK<Cfg>, grid, dim3(Cfg::THREADS), A, B, C, M, N, K);
}

// Returns the number of shapes on which the kernel produced at least one bad element.
int runAll(const char* name, Runner run)
{
    int failing = 0;
    double worst = 0.0;
    std::string first;
    for (const Shape& s : testShapes()) {
        const std::vector<float> A = testMatrix(s.M, s.K, 1u);
        const std::vector<float> B = testMatrix(s.K, s.N, 2u);
        std::vector<float> C(static_cast<std::size_t>(s.M) * s.N, -7.0f);
        run(A.data(), B.data(), C.data(), s.M, s.N, s.K);
        const Verdict v = checkGemm(A, B, C, s.M, s.N, s.K);
        worst = v.worst > worst ? v.worst : worst;
        if (v.bad > 0) {
            if (failing == 0) {
                first = std::to_string(s.M) + "x" + std::to_string(s.N) + "x" + std::to_string(s.K) +
                        " (" + std::to_string(v.bad) + " bad)";
            }
            ++failing;
        }
    }
    std::printf("  %-28s shapes passed %2zu/%zu  worst err/bound %.3f%s%s\n", name,
                testShapes().size() - failing, testShapes().size(), worst,
                failing ? "  first failure " : "", first.c_str());
    return failing;
}

int main()
{
    int problems = 0;
    std::printf("platform: %s\n\n1. instances\n", MP4_PLATFORM);
    for (const InstanceInfo& i : kInstances) {
        std::printf("  %-18s work-items %3d  LDS %5d bytes  FMAs per LDS read %.2f\n", i.name,
                    i.threads, i.ldsBytes, static_cast<double>(i.tm * i.tn) / (i.tm + i.tn));
    }

    std::printf("\n2. tuning table\n");
    for (std::size_t r = 0; r < sizeof(kTuning) / sizeof(kTuning[0]); ++r) {
        const TuningRow& t = kTuning[r];
        bool ok = t.instance >= 0 && t.instance < kInstanceCount;
        for (std::size_t q = 0; q < r; ++q) {
            ok = ok && std::string(kTuning[q].arch) != t.arch;
        }
        const int threads = ok ? kInstances[t.instance].threads : 0;
        ok = ok && threads % t.waveWidth == 0;
        std::printf("  %-8s wave %2d  %-18s lane use %5.1f %%  %s  [%s]\n", t.arch, t.waveWidth,
                    ok ? kInstances[t.instance].name : "?", ok ? 100.0 * laneUse(threads, t.waveWidth) : 0.0,
                    ok ? "ok" : "INVALID", t.evidence);
        problems += ok ? 0 : 1;
    }
    const struct { const char* name; int major, minor; bool nv; } probes[] = {
        {"gfx90a:sramecc+:xnack-", 0, 0, false}, {"gfx1100", 0, 0, false},
        {"", 8, 0, true}, {"gfx803", 0, 0, false},
    };
    for (const auto& p : probes) {
        const std::string key = archKey(p.name, p.major, p.minor, p.nv);
        const TuningRow* row = findTuning(key);
        std::printf("  archKey(\"%s\", %d, %d, %s) = %-7s -> %s\n", p.name, p.major, p.minor,
                    p.nv ? "nvidia" : "amd", key.c_str(),
                    row ? kInstances[row->instance].name : "NO ROW: the program must stop here");
    }

    std::printf("\n3. the kernel, unchanged, in the CPU emulator: %zu shapes, fp32, MP3 tolerance\n",
                testShapes().size());
    problems += runAll(kInstances[0].name, &launchCfg<Inst0>);
    problems += runAll(kInstances[1].name, &launchCfg<Inst1>);
    problems += runAll(kInstances[2].name, &launchCfg<Inst2>);

    std::printf("\n4. seeded fault (must be caught)\n");
    const int caught = runAll("clamp k (i0 instance)", &runClampK<Inst0>);
    if (caught == 0) {
        std::printf("  NOT caught: the shape matrix cannot see this bug\n");
        ++problems;
    }

    std::printf("\nresult: %s (%d problem(s))\n", problems == 0 ? "PASS" : "FAIL", problems);
    return problems == 0 ? 0 : 1;
}

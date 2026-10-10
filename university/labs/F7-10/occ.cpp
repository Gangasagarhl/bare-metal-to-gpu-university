// F7-10 Listing 1: an occupancy calculator for gfx90a, using the limits that this build's
// compiler applies (inferred from its own reports, run.sh step occ_check), not a datasheet.
// Input lines: name  vgprs(total, incl. AGPRs)  lds_bytes_per_workgroup  workgroup_size  [reported]
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

struct Target {
    int vgprBudget;      // VGPRs per lane available to one SIMD's waves
    int vgprGranule;     // allocation unit
    int maxWavesPerSimd; // hardware wave slots per SIMD
    int simdsPerCu;
    int ldsPerCu;        // bytes
    int waveSize;
};

// Model values for gfx90a as the compiler applies them (see the chapter's unverified box).
constexpr Target kGfx90a{512, 8, 8, 4, 65536, 64};

int roundUp(int v, int g)
{
    return (v + g - 1) / g * g;
}

int main()
{
    const Target t = kGfx90a;
    std::printf("%-26s %5s %6s %5s | %8s %8s | %9s %8s %s\n", "kernel", "vgpr", "lds", "wg",
                "by vgpr", "by lds", "predicted", "reported", "limiter");
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream in(line);
        std::string name;
        int vgpr = 0, lds = 0, wg = 0;
        std::string reported = "-";
        in >> name >> vgpr >> lds >> wg >> reported;
        const int byVgpr = std::min(t.maxWavesPerSimd, t.vgprBudget / roundUp(std::max(vgpr, 1), t.vgprGranule));
        int byLds = t.maxWavesPerSimd;
        if (lds > 0) {
            const int groupsPerCu = t.ldsPerCu / lds;
            const int wavesPerGroup = (wg + t.waveSize - 1) / t.waveSize;
            byLds = std::min(t.maxWavesPerSimd, groupsPerCu * wavesPerGroup / t.simdsPerCu);
        }
        const int predicted = std::min(byVgpr, byLds);
        const char* limiter = predicted == t.maxWavesPerSimd ? "wave slots"
                              : (byVgpr <= byLds ? "VGPRs" : "LDS");
        std::string verdict;
        if (reported != "-") {
            verdict = (std::to_string(predicted) == reported) ? "  match" : "  MISMATCH";
        }
        std::printf("%-26s %5d %6d %5d | %8d %8d | %9d %8s %s%s\n", name.c_str(), vgpr, lds, wg,
                    byVgpr, byLds, predicted, reported.c_str(), limiter, verdict.c_str());
    }
    return 0;
}

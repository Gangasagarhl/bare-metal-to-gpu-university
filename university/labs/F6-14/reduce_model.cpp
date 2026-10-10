// F6-14 Listing 3: what the shared-memory steps of v1, v2 and v3 do inside ONE block of 256
// threads (8 warps of 32), step by step. For each step it counts:
//   active  - threads that do an addition
//   warps   - warps with at least one active thread (they must issue the step)
//   mixed   - warps where some lanes are active and some are not (divergence)
//   passes  - worst bank passes of one warp's reads of s[a] (model: 32 banks x 4 bytes)
#include <algorithm>
#include <cstdio>
#include <map>
#include <set>
#include <vector>

constexpr int B = 256;

int bankPasses(const std::vector<int>& words)
{
    std::map<int, std::set<int>> perBank;
    for (int w : words) { perBank[w % 32].insert(w); }
    std::size_t worst = 0;
    for (const auto& kv : perBank) { worst = std::max(worst, kv.second.size()); }
    return static_cast<int>(worst);
}

void model(int version)
{
    std::printf("v%d  %-8s %-7s %-6s %-6s %s\n", version, "stride", "active", "warps", "mixed", "passes");
    std::vector<int> strides;
    if (version == 3) { for (int s = B / 2; s > 0; s /= 2) { strides.push_back(s); } }
    else { for (int s = 1; s < B; s *= 2) { strides.push_back(s); } }
    int totalWarpSteps = 0;
    for (int s : strides) {
        int active = 0, warps = 0, mixed = 0, worst = 0;
        for (int w = 0; w < B / 32; ++w) {
            int on = 0;
            std::vector<int> words;                        // shared words read as s[a] by active lanes
            for (int lane = 0; lane < 32; ++lane) {
                int tid = w * 32 + lane;
                bool act = false;
                int a = 0;
                if (version == 1) { act = tid % (2 * s) == 0; a = tid; }
                if (version == 2) { a = 2 * s * tid; act = a < B; }
                if (version == 3) { act = tid < s; a = tid; }
                if (act) { ++on; words.push_back(a); }
            }
            active += on;
            warps += (on > 0);
            mixed += (on > 0 && on < 32);
            if (!words.empty()) { worst = std::max(worst, bankPasses(words)); }
        }
        totalWarpSteps += warps;
        std::printf("    %-8d %-7d %-6d %-6d %d\n", s, active, warps, mixed, worst);
    }
    std::printf("    warp-steps issued in the shared-memory phase: %d\n\n", totalWarpSteps);
}

int main()
{
    for (int v = 1; v <= 3; ++v) { model(v); }
    return 0;
}

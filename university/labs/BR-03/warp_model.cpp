// BR-03 Listing 7: a MODEL (plain C++, not a GPU) of two group effects that CPU threads do not have.
// Part 1, divergence: Listing 6's branch. Every byte costs kCommon model instructions; a byte >= 200
//   also costs kExtra. A CPU thread pays only for its own bytes. A group of `width` lanes issues the
//   extra path once if ANY of its lanes needs it, and every lane of the group waits for it.
// Part 2, access pattern: which 32-byte segments one load instruction of a group touches when lane l
//   reads byte l (interleaved) or byte l * K (chunked, K bytes per thread). Segment size as in the
//   TG-1 teaching model of F6-08 (an assumption of the model, not a fact about one GPU).
#include <algorithm>
#include <cstdio>
#include <set>
#include <vector>

constexpr long kCommon = 6;
constexpr long kExtra = 34;
constexpr long kSegment = 32;

std::vector<unsigned char> makeInput(std::size_t n)
{
    std::vector<unsigned char> in(n);
    for (std::size_t i = 0; i < n; ++i) {
        in[i] = static_cast<unsigned char>((static_cast<unsigned>(i) * 2654435761u) >> 24);
    }
    return in;
}

void divergence(const char* label, const std::vector<unsigned char>& in, int width)
{
    long useful = 0;       // what CPU threads would execute: each byte its own path
    long laneSlots = 0;    // what a group costs: issued instructions x width
    long mixedGroups = 0;
    const long groups = static_cast<long>(in.size()) / width;
    for (long g = 0; g < groups; ++g) {
        int expensive = 0;
        for (int l = 0; l < width; ++l) {
            expensive += in[static_cast<std::size_t>(g * width + l)] >= 200 ? 1 : 0;
        }
        useful += kCommon * width + kExtra * expensive;
        const long issued = kCommon + (expensive > 0 ? kExtra : 0);
        laneSlots += issued * width;
        mixedGroups += (expensive > 0 && expensive < width) ? 1 : 0;
    }
    const double n = static_cast<double>(in.size());
    std::printf("  %-8s width %2d: %5.1f %% of groups mixed; per byte: useful %5.2f, paid %5.2f; "
                "SIMD efficiency %5.1f %%\n", label, width, 100.0 * static_cast<double>(mixedGroups) / static_cast<double>(groups),
                static_cast<double>(useful) / n, static_cast<double>(laneSlots) / n,
                100.0 * static_cast<double>(useful) / static_cast<double>(laneSlots));
}

long segmentsPerLoad(int width, long k)
{
    std::set<long> segments;
    for (long l = 0; l < width; ++l) {
        segments.insert((l * k) / kSegment);   // byte address l*k, first step of every lane
    }
    return static_cast<long>(segments.size());
}

int main()
{
    std::vector<unsigned char> in = makeInput(std::size_t{1} << 16);
    long rare = 0;
    for (unsigned char b : in) {
        rare += b >= 200 ? 1 : 0;
    }
    std::printf("Part 1: %zu bytes, %ld of them >= 200 (%.1f %%); costs %ld + %ld model instructions\n",
                in.size(), rare, 100.0 * static_cast<double>(rare) / static_cast<double>(in.size()), kCommon, kExtra);
    for (int width : {32, 64}) {
        divergence("as is", in, width);
    }
    std::sort(in.begin(), in.end());
    for (int width : {32, 64}) {
        divergence("sorted", in, width);
    }
    std::printf("Part 2: 32-byte segments touched by ONE load instruction of the group (1-byte loads)\n");
    for (long k : {1L, 4L, 16L, 32L, 64L, 4096L}) {
        std::printf("  lane l reads byte l*%-4ld: width 32 -> %2ld segment(s), width 64 -> %2ld segment(s)%s\n", k,
                    segmentsPerLoad(32, k), segmentsPerLoad(64, k),
                    k == 1 ? "   (interleaved)" : (k == 64 ? "   (Listing 2's histChunked)" : ""));
    }
    return 0;
}

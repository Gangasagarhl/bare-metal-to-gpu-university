// F6-11 Listing 1: a shared-memory bank model for one warp-wide access of 4-byte words.
// Model (parameters, not facts about every GPU; see the chapter's unverified box):
//   BANKS banks, each WIDTH bytes wide; consecutive 4-byte words go to consecutive banks.
//   bank(word) = word % BANKS. Threads that read the SAME word share one read (broadcast).
//   passes = the largest number of DISTINCT words that fall into one bank.
#include <cstdio>
#include <functional>
#include <map>
#include <numeric>
#include <set>
#include <string>

constexpr int BANKS = 32;
constexpr int WIDTH = 4;
constexpr int LANES = 32;

int passes(const std::function<long(int)>& byteAddressOfLane)
{
    std::map<long, std::set<long>> wordsInBank;
    for (int lane = 0; lane < LANES; ++lane) {
        long word = byteAddressOfLane(lane) / WIDTH;
        wordsInBank[word % BANKS].insert(word);
    }
    std::size_t worst = 0;
    for (const auto& kv : wordsInBank) { worst = std::max(worst, kv.second.size()); }
    return static_cast<int>(worst);
}

int main()
{
    std::printf("model: %d banks x %d bytes, %d lanes, 4-byte elements\n\n", BANKS, WIDTH, LANES);
    std::printf("%-44s %-7s %s\n", "access pattern (lane = 0..31)", "passes", "gcd(stride,32)");
    for (int s : {1, 2, 3, 4, 5, 8, 16, 17, 32, 33}) {
        std::string name = "a[lane * " + std::to_string(s) + "]";
        std::printf("%-44s %-7d %d\n", name.c_str(), passes([s](int l) { return 4L * l * s; }),
                    std::gcd(s, 32));
    }
    std::printf("\n%-44s %-7s\n", "special cases", "passes");
    std::printf("%-44s %-7d\n", "a[0] (every lane the same word: broadcast)", passes([](int) { return 0L; }));
    std::printf("%-44s %-7d\n", "a[lane / 2] (pairs share a word)", passes([](int l) { return 4L * (l / 2); }));
    std::printf("%-44s %-7d\n", "char c[lane] (four lanes per word)", passes([](int l) { return 1L * l; }));
    std::printf("%-44s %-7d\n", "a[(lane * 7) % 32] (a permutation)", passes([](int l) { return 4L * ((l * 7) % 32); }));
    std::printf("\n%-44s %-7s\n", "32 x 32 tile, lane = row, fixed column c = 5", "passes");
    std::printf("%-44s %-7d\n", "tile[lane][5], pitch 32", passes([](int l) { return 4L * (l * 32 + 5); }));
    std::printf("%-44s %-7d\n", "tile[lane][5], pitch 33 (padding)", passes([](int l) { return 4L * (l * 33 + 5); }));
    std::printf("%-44s %-7d\n", "tile[lane][5 ^ lane], pitch 32 (XOR swizzle)",
                passes([](int l) { return 4L * (l * 32 + (5 ^ l)); }));
    std::printf("%-44s %-7d\n", "tile[5][lane ^ 5], pitch 32 (swizzled row)",
                passes([](int l) { return 4L * (5 * 32 + (l ^ 5)); }));
    return 0;
}

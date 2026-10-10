// quorum.cpp - DS403 F5-47 worked example: cut a cluster of n nodes into two groups in
// every possible way and ask which group may still decide, under two rules:
//   "majority": a group may decide only if it has more than n/2 nodes;
//   "lowest id": every non-empty group decides (it elects its own lowest id as leader).
#include <cstdio>
#include <string>

namespace {
std::string members(unsigned mask, int n)
{
    std::string s = "{";
    for (int i = 0; i < n; ++i)
        if (mask & (1u << i)) { if (s.size() > 1) s += ","; s += std::to_string(i + 1); }
    return s + "}";
}
}  // namespace

int main()
{
    for (int n = 3; n <= 5; ++n) {
        const unsigned all = (1u << n) - 1;
        int cuts = 0, both_majority = 0, none_majority = 0, both_lowest = 0;
        std::printf("n = %d (majority = %d)\n", n, n / 2 + 1);
        // Each cut counted once: group A always contains node 1.
        for (unsigned a = 1; a < all; ++a) {
            if ((a & 1u) == 0) continue;
            unsigned b = all & ~a;
            int ca = __builtin_popcount(a), cb = __builtin_popcount(b);
            bool ma = ca > n / 2, mb = cb > n / 2;
            ++cuts;
            if (ma && mb) ++both_majority;
            if (!ma && !mb) ++none_majority;
            ++both_lowest;                                 // both groups are non-empty
            if (n == 3 || n == 4)
                std::printf("  %-12s | %-12s  majority rule: %-14s lowest-id rule: both sides decide\n",
                            members(a, n).c_str(), members(b, n).c_str(),
                            ma ? "left decides" : (mb ? "right decides" : "nobody decides"));
        }
        std::printf("  cuts: %d; both sides decide: majority %d, lowest-id %d; nobody decides: majority %d\n",
                    cuts, both_majority, both_lowest, none_majority);
    }
    return 0;
}

// Enumerate every allowed ordering of the store-buffering test in two models:
//   SC : each store reaches memory before the same thread's next load (one global order)
//   SB : each store first waits in its thread's store buffer and drains to memory later
// Events: A0 "x = 1", A1 "r1 = y", Af "x drains"; B0 "y = 1", B1 "r2 = x", Bf "y drains".
#include <algorithm>
#include <array>
#include <cstdio>
#include <map>
#include <utility>

enum Event { A0, A1, Af, B0, B1, Bf };

bool before(const std::array<int, 6>& order, int first, int second)
{
    const auto posFirst = std::find(order.begin(), order.end(), first);
    const auto posSecond = std::find(order.begin(), order.end(), second);
    return posFirst < posSecond;
}

void enumerate(bool storeBuffers)
{
    std::array<int, 6> order = {A0, A1, Af, B0, B1, Bf};
    std::map<std::pair<int, int>, int> outcomes;          // (r1, r2) -> number of orderings
    int allowed = 0;
    do {
        // program order, and a store drains only after it was issued
        bool ok = before(order, A0, A1) && before(order, A0, Af) && before(order, B0, B1) &&
                  before(order, B0, Bf);
        if (!storeBuffers) {                               // SC: drain before the next load
            ok = ok && before(order, Af, A1) && before(order, Bf, B1);
        }
        if (!ok) {
            continue;
        }
        ++allowed;
        int memX = 0;
        int memY = 0;
        int r1 = 0;
        int r2 = 0;
        for (const int e : order) {
            switch (e) {
            case Af: memX = 1; break;
            case Bf: memY = 1; break;
            case A1: r1 = memY; break;                     // y is not in A's own buffer
            case B1: r2 = memX; break;                     // x is not in B's own buffer
            default: break;                                // A0, B0 only fill a buffer
            }
        }
        ++outcomes[{r1, r2}];
    } while (std::next_permutation(order.begin(), order.end()));
    std::printf("%s: %d allowed orderings\n", storeBuffers ? "store-buffer model" : "SC model",
                allowed);
    for (const auto& [result, count] : outcomes) {
        std::printf("  r1=%d r2=%d  in %2d orderings\n", result.first, result.second, count);
    }
}

int main()
{
    enumerate(false);
    enumerate(true);
    return 0;
}

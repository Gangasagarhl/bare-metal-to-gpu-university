// F6-18 Listing 2: the 1-bit "split" step that a radix sort repeats, on 8 keys of 3 bits.
// For bit b: flag = 1 if the bit is 0; scan the flags (exclusive) to place the 0-keys;
// the 1-keys go after all 0-keys, in their old order.
#include <cstdio>
#include <vector>

int main()
{
    std::vector<unsigned> keys = {5, 2, 7, 0, 3, 6, 1, 4};
    for (int bit = 0; bit < 3; ++bit) {
        const int n = static_cast<int>(keys.size());
        std::vector<int> isZero(n), scan(n), dst(n);
        int zeros = 0;
        for (int i = 0; i < n; ++i) {
            isZero[i] = ((keys[i] >> bit) & 1u) == 0;
            scan[i] = zeros;                                  // exclusive scan of the 0-flags
            zeros += isZero[i];
        }
        std::vector<unsigned> out(n);
        for (int i = 0; i < n; ++i) {
            int onesBefore = i - scan[i];
            dst[i] = isZero[i] ? scan[i] : zeros + onesBefore;
            out[dst[i]] = keys[i];
        }
        std::printf("bit %d\n  keys      ", bit);
        for (unsigned k : keys) { std::printf(" %u", k); }
        std::printf("\n  bit is 0  ");
        for (int z : isZero) { std::printf(" %d", z); }
        std::printf("\n  excl scan ");
        for (int s : scan) { std::printf(" %d", s); }
        std::printf("   (zeros = %d)\n  dst       ", zeros);
        for (int d : dst) { std::printf(" %d", d); }
        std::printf("\n  result    ");
        for (unsigned k : out) { std::printf(" %u", k); }
        std::printf("\n");
        keys = out;
    }
    return 0;
}

// F6-16 Listing 1: two block-level scan algorithms, step by step, with their work counted.
//   Hillis-Steele: inclusive, log2(n) steps, double-buffered (reads the old array, writes a new one)
//   Blelloch: exclusive, an up-sweep (reduce) then a down-sweep, 2 log2(n) steps, about 2n additions
#include <cstdio>
#include <numeric>
#include <vector>

long hillisSteele(std::vector<long>& a, bool print)
{
    const int n = static_cast<int>(a.size());
    long adds = 0;
    std::vector<long> next(a.size());
    for (int d = 1; d < n; d *= 2) {
        for (int i = 0; i < n; ++i) {                      // every "thread" i, reading the OLD array
            next[i] = (i >= d) ? a[i - d] + a[i] : a[i];
            adds += (i >= d);
        }
        a.swap(next);
        if (print) {
            std::printf("  HS d=%-3d:", d);
            for (long x : a) { std::printf(" %3ld", x); }
            std::printf("\n");
        }
    }
    return adds;
}

long blelloch(std::vector<long>& a, bool print)
{
    const int n = static_cast<int>(a.size());              // n must be a power of two here
    long adds = 0;
    for (int d = 1; d < n; d *= 2) {                       // up-sweep: partial sums in a tree
        for (int i = 2 * d - 1; i < n; i += 2 * d) { a[i] += a[i - d]; ++adds; }
        if (print) {
            std::printf("  up   d=%-3d:", d);
            for (long x : a) { std::printf(" %3ld", x); }
            std::printf("\n");
        }
    }
    a[n - 1] = 0;                                           // clear the root: exclusive scan
    for (int d = n / 2; d >= 1; d /= 2) {                   // down-sweep
        for (int i = 2 * d - 1; i < n; i += 2 * d) {
            long t = a[i - d];
            a[i - d] = a[i];
            a[i] += t;
            ++adds;
        }
        if (print) {
            std::printf("  down d=%-3d:", d);
            for (long x : a) { std::printf(" %3ld", x); }
            std::printf("\n");
        }
    }
    return adds;
}

int main()
{
    const std::vector<long> small = {3, 1, 7, 0, 4, 1, 6, 3};
    std::printf("input:      ");
    for (long x : small) { std::printf(" %3ld", x); }
    std::printf("\n");
    std::vector<long> a = small;
    hillisSteele(a, true);
    std::vector<long> b = small;
    blelloch(b, true);
    std::vector<long> inc(small.size()), exc(small.size());
    std::inclusive_scan(small.begin(), small.end(), inc.begin());
    std::exclusive_scan(small.begin(), small.end(), exc.begin(), 0L);
    std::printf("inclusive matches std::inclusive_scan: %s; exclusive matches std::exclusive_scan: %s\n\n",
                a == inc ? "yes" : "NO", b == exc ? "yes" : "NO");
    std::printf("%-7s %-9s %-14s %-14s %s\n", "n", "steps HS", "additions HS", "additions BL", "sequential");
    for (int n : {8, 32, 256, 1024, 4096}) {
        std::vector<long> x(n, 1), y(n, 1);
        long hs = hillisSteele(x, false);
        long bl = blelloch(y, false);
        int steps = 0;
        for (int d = 1; d < n; d *= 2) { ++steps; }
        std::printf("%-7d %-9d %-14ld %-14ld %d\n", n, steps, hs, bl, n - 1);
    }
    return 0;
}

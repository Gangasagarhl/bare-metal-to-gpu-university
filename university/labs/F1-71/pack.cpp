// F1-71 Listing 1: battery pack arithmetic with a PRETEND cell.
// Pretend cell: nominal 3.6 V, capacity 2.0 Ah. These are exercise numbers, NOT
// the numbers of any real cell or chemistry. Real cell voltages, capacities,
// current limits and safe voltage windows come only from the battery maker.
#include <cstdio>
#include <initializer_list>
#include <utility>

int main()
{
    const double cellV = 3.6;
    const double cellAh = 2.0;
    std::printf("pack layouts (S = cells in series, P = cells in parallel)\n");
    std::printf("%6s %8s %8s %8s\n", "layout", "volts", "Ah", "Wh");
    for (auto [s, p] : {std::pair{1, 1}, std::pair{2, 1}, std::pair{3, 1}, std::pair{3, 2},
                        std::pair{4, 2}}) {
        const double v = s * cellV;
        const double ah = p * cellAh;
        std::printf("   %dS%dP %8.1f %8.1f %8.1f\n", s, p, v, ah, v * ah);
    }
    std::printf("\nC-rate: current = C x capacity in Ah (3S2P pack, 4.0 Ah)\n");
    for (double c : {0.5, 1.0, 2.0, 5.0}) {
        std::printf("  %.1fC -> %5.1f A, ideal time to empty %5.1f min\n", c, c * 4.0, 60.0 / c);
    }
    std::printf("\nrun-time estimate: robot draws 2.5 A on average from the 3S2P pack\n");
    const double usableFraction = 0.8;  // our own design rule, not a battery rule
    std::printf("  ideal: %.2f h; with only %.0f %% used: %.2f h = %.0f min\n", 4.0 / 2.5,
                usableFraction * 100.0, 4.0 * usableFraction / 2.5,
                4.0 * usableFraction / 2.5 * 60.0);
    return 0;
}

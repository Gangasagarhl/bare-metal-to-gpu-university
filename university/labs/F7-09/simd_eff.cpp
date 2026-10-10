// F7-09 Listing 1: what a branch costs in wave32 versus wave64 (a counting model, no timing).
// A wave runs a side of a branch if at least one of its lanes needs it; the lanes that do
// not need it are masked off but their slots are still spent. We count lane-slots.
#include <cmath>
#include <cstdio>
#include <initializer_list>

// Expected lane-slots spent per lane for a two-sided branch:
// every wave runs the common side (cost c) unless no lane needs it (ignored: p small),
// and runs the rare side (cost e) if any of its w lanes needs it.
double slotsPerLane(int w, double p, double c, double e)
{
    const double anyRare = 1.0 - std::pow(1.0 - p, w);       // probability the wave diverges
    return c + e * anyRare;                                   // per lane, since every lane pays
}

int main()
{
    const double c = 10.0;     // instructions on the common side (model value)
    const double e = 100.0;    // instructions on the rare side (model value)
    std::printf("-- part 1: lanes take the rare side independently with probability p\n");
    std::printf("%8s | %18s %18s | %18s %18s\n", "p", "P(diverge) w32", "P(diverge) w64",
                "efficiency w32", "efficiency w64");
    for (double p : {0.0, 0.001, 0.01, 0.05, 0.2, 0.5}) {
        const double useful = c + e * p;                      // what a lane really needs
        const double s32 = slotsPerLane(32, p, c, e);
        const double s64 = slotsPerLane(64, p, c, e);
        std::printf("%8.3f | %17.1f%% %17.1f%% | %17.1f%% %17.1f%%\n", p,
                    100.0 * (1.0 - std::pow(1.0 - p, 32)), 100.0 * (1.0 - std::pow(1.0 - p, 64)),
                    100.0 * useful / s32, 100.0 * useful / s64);
    }

    std::printf("-- part 2: the condition depends on (i / 32) %% 2, i.e. blocks of 32 alternate\n");
    for (int w : {32, 64}) {
        int diverged = 0;
        const int waves = 1024 / w;
        for (int wave = 0; wave < waves; ++wave) {
            bool sawTrue = false;
            bool sawFalse = false;
            for (int lane = 0; lane < w; ++lane) {
                const int i = wave * w + lane;
                if ((i / 32) % 2 == 0) {
                    sawTrue = true;
                } else {
                    sawFalse = true;
                }
            }
            if (sawTrue && sawFalse) {
                ++diverged;
            }
        }
        std::printf("width %d: %2d of %2d waves diverge\n", w, diverged, waves);
    }
    return 0;
}

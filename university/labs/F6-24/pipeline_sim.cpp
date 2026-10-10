// F6-24 Listing 4: a timeline model of one block working through K/BK tiles.
// Single buffer: load tile t, wait, compute tile t, then start loading tile t+1.
// Double buffer: the load of tile t+1 is issued before computing tile t.
// Load latency and compute time per tile are MODEL INPUTS, not GPU measurements.
#include <algorithm>
#include <cstdio>
#include <iostream>

int main()
{
    int tiles = 0;
    std::cin >> tiles;
    std::printf("%d tiles; times in cycles of the model\n", tiles);
    std::printf("%8s %8s | %12s %12s %8s\n", "load", "compute", "single", "double", "speed-up");
    long load = 0, compute = 0;
    while (std::cin >> load >> compute) {
        const long single = tiles * (load + compute);
        long loadDone = load;                     // tile 0 arrives
        long t = 0;
        for (int i = 0; i < tiles; ++i) {
            t = std::max(t, loadDone);            // wait for tile i
            const long nextIssue = t;             // the copy of tile i+1 starts now ...
            t += compute;                         // ... while tile i is computed
            loadDone = nextIssue + load;
        }
        std::printf("%8ld %8ld | %12ld %12ld %7.2fx\n", load, compute, single, t,
                    double(single) / double(t));
    }
    return 0;
}

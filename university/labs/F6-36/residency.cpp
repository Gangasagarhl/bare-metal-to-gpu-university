// F6-36 Listing 2: why a grid-wide barrier needs every block resident at once.
// A model of the block scheduler of TG-1, the university's invented teaching GPU (4 SMs; how
// many blocks fit on one SM depends on the kernel's registers and shared memory). Blocks are
// placed in free slots in order. Each block works, then waits at grid.sync(), keeping its slot.
// The barrier opens only when every block of the grid has arrived.
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::string name;
    int sms = 0;
    int perSm = 0;
    int grid = 0;
    while (std::cin >> name >> sms >> perSm >> grid) {
        if (sms <= 0 || perSm <= 0 || grid <= 0) {
            std::printf("%s: invalid input\n", name.c_str());
            return 1;
        }
        const int slots = sms * perSm;
        std::printf("%s: %d SMs x %d resident blocks per SM = %d slots; grid of %d blocks\n",
                    name.c_str(), sms, perSm, slots, grid);
        std::printf("  cooperative-launch rule: grid <= %d -> %s\n", slots,
                    grid <= slots ? "launch accepted" : "launch refused (too large)");
        // what happens if the grid is launched anyway, with an ordinary launch
        std::vector<int> state(static_cast<std::size_t>(grid), 0);   // 0 waiting to start, 1 at barrier, 2 done
        int started = 0;
        int atBarrier = 0;
        int finished = 0;
        int freeSlots = slots;
        bool released = false;
        for (int round = 1; round <= 4 && finished < grid; ++round) {
            int startedNow = 0;
            while (freeSlots > 0 && started < grid) {        // scheduler fills free slots
                state[static_cast<std::size_t>(started)] = 1;
                ++started;
                ++atBarrier;
                --freeSlots;
                ++startedNow;
            }
            if (!released && atBarrier == grid) {
                released = true;                             // every block arrived: barrier opens
            }
            int finishedNow = 0;
            if (released) {
                for (int& s : state) {
                    if (s == 1) {
                        s = 2;
                        ++finished;
                        ++freeSlots;
                        ++finishedNow;
                    }
                }
                atBarrier = 0;
            }
            std::printf("  round %d: %d blocks started, %d waiting at grid.sync, %d never started, %d finished\n",
                        round, startedNow, released ? 0 : atBarrier, grid - started, finished);
            if (!released && startedNow == 0) {
                std::printf("  no free slot and no block can leave the barrier: the kernel hangs\n");
                break;
            }
        }
        if (finished == grid) {
            std::printf("  all %d blocks finished\n", grid);
        }
    }
    return 0;
}

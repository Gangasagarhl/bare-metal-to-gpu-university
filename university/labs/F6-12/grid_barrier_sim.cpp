// F6-12 Listing 2: why a home-made barrier across the whole grid can hang.
// A toy GPU (TG-1, invented: 2 SMs x 4 resident blocks = 8 slots) runs a grid whose blocks
// each do some work, then arrive at a global counter and spin until all gridDim blocks arrived.
// Blocks get a slot in blockIdx order and keep it until they finish (no pre-emption in this model).
#include <cstdio>
#include <vector>

enum class Phase { Waiting, Working, Spinning, Done };

void run(int gridBlocks, int slots)
{
    std::vector<Phase> phase(gridBlocks, Phase::Waiting);
    std::vector<int> workLeft(gridBlocks, 3);       // 3 steps of work per block
    int arrived = 0, nextToStart = 0, resident = 0;
    for (int step = 1; step <= 1000; ++step) {
        while (resident < slots && nextToStart < gridBlocks) {   // the block scheduler fills free slots
            phase[nextToStart++] = Phase::Working;
            ++resident;
        }
        bool progress = false;
        for (int b = 0; b < gridBlocks; ++b) {
            if (phase[b] == Phase::Working) {
                if (--workLeft[b] == 0) { phase[b] = Phase::Spinning; ++arrived; }
                progress = true;
            } else if (phase[b] == Phase::Spinning && arrived == gridBlocks) {
                phase[b] = Phase::Done;
                --resident;
                progress = true;
            }
        }
        int done = 0;
        for (Phase p : phase) { done += (p == Phase::Done); }
        if (done == gridBlocks) {
            std::printf("grid %3d blocks, %d slots: finished after %d steps\n", gridBlocks, slots, step);
            return;
        }
        if (!progress) {
            int spinning = 0;
            for (Phase p : phase) { spinning += (p == Phase::Spinning); }
            std::printf("grid %3d blocks, %d slots: HANG at step %d: %d blocks spin at the barrier, "
                        "%d block(s) never got a slot (arrived %d of %d)\n",
                        gridBlocks, slots, step, spinning, gridBlocks - nextToStart, arrived, gridBlocks);
            return;
        }
    }
    std::printf("grid %3d blocks: step limit reached\n", gridBlocks);
}

int main()
{
    const int sms = 2, blocksPerSm = 4;               // TG-1: invented limits
    const int slots = sms * blocksPerSm;
    std::printf("TG-1 (invented): %d SMs x %d resident blocks = %d slots\n", sms, blocksPerSm, slots);
    for (int grid : {4, 8, 9, 16, 64}) { run(grid, slots); }
    std::printf("cooperative-launch rule: grid <= blocks per SM x SMs = %d\n", slots);
    return 0;
}

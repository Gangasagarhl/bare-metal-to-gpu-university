// F1-56 Listing 3: how the blocks of one launch spread over the SMs (or CUs).
// Input lines: label SMs blocksPerSM gridBlocks
//   SMs          number of SMs/CUs (from YOUR device query or the vendor document)
//   blocksPerSM  how many blocks of this kernel fit on one SM at the same time
//   gridBlocks   how many blocks the launch creates
// Model: blocks are handed out to free slots; a "wave" is one full round of slots.
#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    std::string label;
    long sms = 0, perSm = 0, grid = 0;
    std::printf("%-26s %4s %6s %6s %6s %7s %s\n", "case", "SMs", "slots", "blocks", "waves", "busy %", "last wave");
    while (std::cin >> label >> sms >> perSm >> grid) {
        if (sms <= 0 || perSm <= 0 || grid <= 0) {
            std::printf("%-26s invalid input\n", label.c_str());
            continue;
        }
        const long slots = sms * perSm;                  // blocks resident at once
        const long waves = (grid + slots - 1) / slots;   // rounds needed (rounded up)
        const long last = grid - (waves - 1) * slots;    // blocks in the final round
        // Share of slot-rounds that hold a block, assuming equal block durations.
        const double busy = 100.0 * static_cast<double>(grid) / static_cast<double>(waves * slots);
        std::printf("%-26s %4ld %6ld %6ld %6ld %7.1f %ld of %ld slots\n",
                    label.c_str(), sms, slots, grid, waves, busy, last, slots);
    }
    return 0;
}

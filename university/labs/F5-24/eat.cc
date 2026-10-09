// eat.cc - allocates and touches memory 4 MiB at a time, reporting as it goes.
// Run inside a container with a memory limit by run.sh (F5-24 forensic lab).
#include <cstdio>
#include <cstring>
#include <vector>

int main()
{
    std::vector<std::vector<char>> blocks;
    for (int i = 1; i <= 64; ++i) {                  // up to 256 MiB in total
        blocks.emplace_back(4u << 20);
        std::memset(blocks.back().data(), 1, blocks.back().size());  // touch every page
        if (i % 4 == 0) {                            // report every 16 MiB
            std::printf("allocated %d MiB\n", 4 * i);
            std::fflush(stdout);
        }
    }
    std::printf("done: 256 MiB allocated and touched\n");
    return 0;
}

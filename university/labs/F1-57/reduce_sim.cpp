// F1-57 forensic evidence program: a host model of a "warp sum" written for
// 32-lane warps (shuffle-down steps 16, 8, 4, 2, 1), run on groups of 32 and 64 lanes.
// Each lane holds the value 1, so the correct group sum equals the group width.
#include <cstdio>
#include <vector>

// Model of a shuffle-down tree: after each step, lane l adds the value of lane l + offset.
int warpSumHardcoded(std::vector<int> lane)
{
    const int width = static_cast<int>(lane.size());
    for (int offset = 16; offset > 0; offset /= 2) {        // BUG under test: 16 = 32 / 2 hard-coded
        std::vector<int> next = lane;
        for (int l = 0; l < width; ++l) {
            if (l + offset < width) { next[static_cast<std::size_t>(l)] += lane[static_cast<std::size_t>(l + offset)]; }
        }
        lane = next;
    }
    return lane[0];                                         // lane 0 holds the result
}

int warpSumPortable(std::vector<int> lane)
{
    const int width = static_cast<int>(lane.size());
    for (int offset = width / 2; offset > 0; offset /= 2) { // starts from the real width
        std::vector<int> next = lane;
        for (int l = 0; l < width; ++l) {
            if (l + offset < width) { next[static_cast<std::size_t>(l)] += lane[static_cast<std::size_t>(l + offset)]; }
        }
        lane = next;
    }
    return lane[0];
}

int main()
{
    for (int width : {32, 64}) {
        std::vector<int> ones(static_cast<std::size_t>(width), 1);
        std::printf("group width %2d: expected %2d, hard-coded tree gives %2d, portable tree gives %2d\n",
                    width, width, warpSumHardcoded(ones), warpSumPortable(ones));
    }
    return 0;
}

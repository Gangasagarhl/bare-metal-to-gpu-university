// F7-08 Listing 1: how one workgroup is cut into lock-step groups of W lanes,
// and what a reduction that hard-codes 32 does inside a 64-lane wavefront.
// A host-only model: the "lanes" are array slots, the "shuffle" is an array read.
#include <cstdio>
#include <vector>

// Split a workgroup of `threads` work-items into groups of `width` lanes.
void splitWorkgroup(int threads, int width)
{
    const int groups = (threads + width - 1) / width;          // round up
    const int idle = groups * width - threads;                  // lanes with no work-item
    const double used = 100.0 * threads / (groups * width);
    std::printf("workgroup %4d, width %2d: %2d groups, %2d idle lanes, %5.1f %% of lanes used\n",
                threads, width, groups, idle, used);
}

// One lock-step "shuffle down": lane l reads the value of lane l + offset (if it exists).
std::vector<int> shuffleDown(const std::vector<int>& v, int offset)
{
    std::vector<int> r(v.size());
    for (std::size_t l = 0; l < v.size(); ++l) {
        const std::size_t src = l + static_cast<std::size_t>(offset);
        r[l] = src < v.size() ? v[src] : v[l];                   // out of range: keep own value
    }
    return r;
}

// Tree reduction inside one wave; `firstOffset` is where the halving starts.
std::vector<int> waveReduce(std::vector<int> v, int firstOffset)
{
    for (int offset = firstOffset; offset > 0; offset /= 2) {
        const std::vector<int> other = shuffleDown(v, offset);
        for (std::size_t l = 0; l < v.size(); ++l) {
            v[l] += other[l];
        }
    }
    return v;
}

int main()
{
    std::printf("-- part 1: one workgroup, cut into warps (32) or wavefronts (64)\n");
    for (int threads : {64, 96, 128, 192, 256, 1000}) {
        splitWorkgroup(threads, 32);
        splitWorkgroup(threads, 64);
    }

    std::printf("-- part 2: thread -> (group, lane) for a few work-items of a 256-item workgroup\n");
    for (int t : {0, 31, 32, 63, 64, 100, 255}) {
        std::printf("threadIdx.x %3d: warp %d lane %2d | wavefront %d lane %2d\n",
                    t, t / 32, t % 32, t / 64, t % 64);
    }

    std::printf("-- part 3: sum of 1..W inside one group; the answer is read from lane 0\n");
    for (int width : {32, 64}) {
        std::vector<int> v(static_cast<std::size_t>(width));
        for (int l = 0; l < width; ++l) {
            v[static_cast<std::size_t>(l)] = l + 1;
        }
        const int expected = width * (width + 1) / 2;
        const std::vector<int> hard = waveReduce(v, 16);          // "offset = 16" copied from CUDA code
        const std::vector<int> port = waveReduce(v, width / 2);   // offset = warpSize / 2
        std::printf("width %2d: expected %4d | hard-coded 16: lane 0 holds %4d | warpSize/2: lane 0 holds %4d -> %s\n",
                    width, expected, hard[0], port[0],
                    hard[0] == expected ? "both correct" : "hard-coded version WRONG");
        if (width > 32) {
            std::printf("          the hard-coded version left the other half in lane 32: %d (528 + %d = %d)\n",
                        hard[32], hard[32], 528 + hard[32]);
        }
    }
    return 0;
}

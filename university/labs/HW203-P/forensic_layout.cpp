// Evidence for the HW203 final's forensic question "The energy sum that read eight times more".
// A particle simulator sums the mass of every particle twice per step. Two layouts of the same
// data are traced through the course cache model (cache.hpp, F1-33: tags only, exact LRU):
//   AoS: one 64-byte struct per particle {x, y, z, vx, vy, vz, mass, charge}; mass at offset 48
//   SoA: all masses in one array of doubles
// Geometries: the L1 data cache the build container reports (64 sets x 12 ways x 64 B = 48 KiB)
// and a fully associative cache of the same capacity (1 set x 768 ways), to tell capacity misses
// from conflict misses. Every number printed is a model count, not a hardware counter reading.
#include <cstdint>
#include <cstdio>

#include "cache.hpp"

struct Counts
{
    std::uint64_t accesses = 0;
    std::uint64_t pass1 = 0;
    std::uint64_t pass2 = 0;
};

Counts trace(unsigned sets, unsigned ways, unsigned particles, unsigned strideBytes, unsigned offset)
{
    Cache cache(sets, ways, 64);
    Counts c;
    for (unsigned p = 0; p < particles; ++p) {           // pass 1
        cache.access(std::uint64_t{p} * strideBytes + offset);
        ++c.accesses;
    }
    c.pass1 = cache.misses();
    for (unsigned p = 0; p < particles; ++p) {           // pass 2, same data again
        cache.access(std::uint64_t{p} * strideBytes + offset);
        ++c.accesses;
    }
    c.pass2 = cache.misses() - c.pass1;
    return c;
}

int main()
{
    std::printf("struct Particle {x, y, z, vx, vy, vz, mass, charge}: 8 doubles = 64 bytes; mass at offset 48\n");
    std::printf("%-9s %-24s %-6s %-9s %-13s %-13s %-13s\n", "particles", "cache", "layout", "data",
                "accesses", "misses pass1", "misses pass2");
    for (const unsigned particles : {512u, 4096u}) {
        struct Geometry
        {
            unsigned sets, ways;
            const char* name;
        };
        for (const Geometry& g : {Geometry{64, 12, "64 sets x 12 ways (L1d)"},
                                  Geometry{1, 768, "1 set x 768 ways (full)"}}) {
            const unsigned sets = g.sets;
            const unsigned ways = g.ways;
            const char* name = g.name;
            const Counts aos = trace(sets, ways, particles, 64, 48);
            const Counts soa = trace(sets, ways, particles, 8, 0);
            std::printf("%-9u %-24s %-6s %5u KiB %-13llu %-13llu %-13llu\n", particles, name, "AoS",
                        particles * 64 / 1024, static_cast<unsigned long long>(aos.accesses),
                        static_cast<unsigned long long>(aos.pass1),
                        static_cast<unsigned long long>(aos.pass2));
            std::printf("%-9u %-24s %-6s %5u KiB %-13llu %-13llu %-13llu\n", particles, name, "SoA",
                        particles * 8 / 1024, static_cast<unsigned long long>(soa.accesses),
                        static_cast<unsigned long long>(soa.pass1),
                        static_cast<unsigned long long>(soa.pass2));
        }
    }
    std::printf("(model counts from cache.hpp; AoS reads one 8-byte field per 64-byte struct, SoA reads consecutive doubles)\n");
    return 0;
}

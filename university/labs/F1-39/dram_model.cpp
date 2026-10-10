// A teaching model of DRAM: 2 channels x 4 banks, each bank with one open row (row buffer).
// Address mapping (exercise geometry, not any real product): bits 5..0 byte in a 64-byte block,
// bit 6 channel, bits 13..7 column block, bits 15..14 bank, bits 16 and up row.
// Exercise costs: a row hit costs 1 time unit, a row miss (close + open another row) 3 units.
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct Where
{
    unsigned channel, bank, row, column;
};

Where map(std::uint64_t address)
{
    const auto bits = [address](unsigned shift, std::uint64_t mask) {
        return static_cast<unsigned>((address >> shift) & mask);
    };
    return Where{bits(6, 1), bits(14, 3), bits(16, 0xFFFF), bits(7, 127)};
}

void run(const std::string& name, const std::vector<std::uint64_t>& addresses, bool show)
{
    std::array<std::array<long, 4>, 2> openRow;              // -1 = no row open yet
    for (auto& ch : openRow) {
        ch.fill(-1);
    }
    int hits = 0;
    int misses = 0;
    std::array<int, 2> perChannel{};
    for (const std::uint64_t a : addresses) {
        const Where w = map(a);
        long& open = openRow[w.channel][w.bank];
        const bool hit = open == static_cast<long>(w.row);
        hit ? ++hits : ++misses;
        open = w.row;
        ++perChannel[w.channel];
        if (show) {
            std::printf("  0x%06llx  channel %u bank %u row %3u column %3u  %s\n",
                        static_cast<unsigned long long>(a), w.channel, w.bank, w.row, w.column,
                        hit ? "row hit" : "row miss");
        }
    }
    std::printf("%-18s accesses %3zu  row hits %3d  row misses %3d  channel 0/1: %d/%d  time %d\n",
                name.c_str(), addresses.size(), hits, misses, perChannel[0], perChannel[1],
                hits * 1 + misses * 3);
}

int main()
{
    std::vector<std::uint64_t> first, sequential, scattered, sameBank;
    for (std::uint64_t i = 0; i < 6; ++i) {
        first.push_back(i * 64);                              // six neighbouring blocks
    }
    for (std::uint64_t i = 0; i < 256; ++i) {
        sequential.push_back(i * 64);                         // 16 KiB read in order
        scattered.push_back((i * 2654435761u) % (1u << 22) & ~std::uint64_t{63});  // pseudo-random
        sameBank.push_back(i * (1u << 16));                   // stride 64 KiB
    }
    std::printf("first six blocks:\n");
    run("six neighbours", first, true);
    std::printf("256 accesses each:\n");
    run("sequential 64 B", sequential, false);
    run("scattered", scattered, false);
    run("stride 64 KiB", sameBank, false);
    return 0;
}

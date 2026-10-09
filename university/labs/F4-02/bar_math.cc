// bar_math.cc - DR301 F4-02 worked example: decode BAR values read during sizing.
// Input lines: "<name> <original low> <after writing ones, low> [<original high> <after ones, high>]"
// (hex). The arithmetic is the same as pci.cc's size_bars().
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

int main()
{
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string name;
        uint32_t orig = 0, probe = 0, orig_hi = 0, probe_hi = 0;
        in >> name >> std::hex >> orig >> probe;
        const bool io = orig & 0x1;
        if (io) {
            const uint32_t mask = probe & ~0x3u;
            std::printf("%-6s I/O   : address 0x%x, mask 0x%08x -> size 0x%x (16-bit I/O space)\n",
                        name.c_str(), orig & ~0x3u, mask, (~mask + 1) & 0xFFFF);
            continue;
        }
        const unsigned type = (orig >> 1) & 0x3;
        const bool prefetch = orig & 0x8;
        uint64_t addr = orig & ~0xFu, mask = probe & ~0xFu;
        if (type == 2) {                         // 64-bit: the next BAR is the upper half
            in >> orig_hi >> probe_hi;
            addr |= uint64_t{orig_hi} << 32;
            mask |= uint64_t{probe_hi} << 32;
        } else {
            mask |= 0xFFFFFFFF00000000ull;
        }
        std::printf("%-6s mem%s: type bits %u%s, address 0x%llx, mask 0x%016llx -> size 0x%llx\n",
                    name.c_str(), type == 2 ? "64" : "32", type, prefetch ? ", prefetchable" : "",
                    static_cast<unsigned long long>(addr), static_cast<unsigned long long>(mask),
                    static_cast<unsigned long long>(~mask + 1));
    }
    return 0;
}

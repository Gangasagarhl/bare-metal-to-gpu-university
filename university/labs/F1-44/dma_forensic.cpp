// F1-44 forensic evidence: a receive path that logs every packet it hands to
// the network stack. The device DMAs four 32-byte packets into the SAME buffer
// (bytes 0..31), one after another. The driver's "peek" at the packet header
// (first 2 bytes) happens before it invalidates; the rest is read after.
#include <cstdio>
#include <vector>

#include "cache_model.h"

int main()
{
    System s;
    for (int pkt = 1; pkt <= 4; ++pkt) {
        for (std::size_t i = 0; i < 32; ++i) {               // device writes packet pkt
            const auto v = static_cast<std::uint8_t>(i < 2 ? pkt : (pkt << 4) | (i & 0xF));
            s.dma_write(i, v);
        }
        // completion interrupt -> driver
        const std::uint8_t hdr0 = s.cpu_read(0);              // peek: header says which packet
        s.invalidate(16, 16);                                 // BUG: only the second line
        std::vector<std::uint8_t> got(32);
        for (std::size_t i = 0; i < 32; ++i) got[i] = s.cpu_read(i);
        char who[40];
        std::snprintf(who, sizeof who, "packet %d (header says %d):", pkt, hdr0);
        dump(who, got.data(), 32);
    }
    return 0;
}

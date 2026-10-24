// HW204 answer keys: every number quoted in university/_keys/HW204.keys.html that is not
// copied from a lab run is recomputed here (Lab Engineer, guide 11.4: "every number in a
// key must come from a real run"). Exercise values throughout (owner ruling A4).
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct Window { const char* name; unsigned base; unsigned size; };

static void decode(const std::vector<Window>& map, unsigned addr)
{
    std::vector<std::string> hits;
    unsigned off = 0;
    for (const Window& w : map) {
        if (addr - w.base < w.size) { hits.push_back(w.name); off = addr - w.base; }
    }
    if (hits.empty()) std::printf("  0x%04X -> bus error (no window)\n", addr);
    else if (hits.size() == 1) std::printf("  0x%04X -> %s offset 0x%X\n", addr, hits[0].c_str(), off);
    else std::printf("  0x%04X -> CONTENTION (%zu windows)\n", addr, hits.size());
}

static void overlaps(const std::vector<Window>& map)
{
    for (std::size_t i = 0; i < map.size(); ++i)
        for (std::size_t j = i + 1; j < map.size(); ++j) {
            const unsigned lo = std::max(map[i].base, map[j].base);
            const unsigned hi = std::min(map[i].base + map[i].size, map[j].base + map[j].size);
            if (lo < hi) std::printf("  overlap %s/%s at 0x%04X-0x%04X\n", map[i].name, map[j].name, lo, hi - 1);
        }
    for (const Window& w : map)
        std::printf("  %s base 0x%04X size 0x%X last 0x%04X aligned-to-size %s\n", w.name, w.base, w.size,
                    w.base + w.size - 1, (w.base % w.size) == 0 ? "yes" : "no");
}

static unsigned bar_size(std::uint32_t lo, bool io, bool is64, std::uint32_t hi, std::uint64_t& out)
{
    if (io) { out = (~(lo & ~0x3u) + 1u) & 0xFFFFFFFFu; return lo & 0x3u; }
    const std::uint32_t flags = lo & 0xFu;
    if (is64) { const std::uint64_t v = (static_cast<std::uint64_t>(hi) << 32) | (lo & ~0xFu); out = ~v + 1; }
    else out = (~(lo & ~0xFu) + 1u) & 0xFFFFFFFFu;
    return flags;
}

static void timer(double fin, double want, int prescaler)
{
    const double counts = fin / prescaler / want;
    const long reload = std::lround(counts) - 1;
    const double actual = fin / prescaler / (reload + 1);
    std::printf("  f_in %.0f Hz, %g Hz wanted, prescaler %d: counts %.3f, reload %ld, fits 16 bits %s, actual %.4f Hz, error %+.4f %%\n",
                fin, want, prescaler, counts, reload, reload <= 65535 ? "yes" : "NO", actual, (actual - want) / want * 100);
}

static void can(std::vector<std::pair<const char*, unsigned>> nodes)
{
    std::vector<bool> alive(nodes.size(), true);
    for (int bit = 10; bit >= 0; --bit) {
        int bus = 1;
        for (std::size_t i = 0; i < nodes.size(); ++i) if (alive[i]) bus &= (nodes[i].second >> bit) & 1;
        for (std::size_t i = 0; i < nodes.size(); ++i)
            if (alive[i] && ((nodes[i].second >> bit) & 1) == 1 && bus == 0) {
                std::printf("  %s (0x%03X) drops out at bit %d\n", nodes[i].first, nodes[i].second, bit); alive[i] = false;
            }
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) if (alive[i]) std::printf("  %s (0x%03X) wins\n", nodes[i].first, nodes[i].second);
}

int main()
{
    std::printf("== Midterm ==\n");
    std::printf("M2: 24 address lines -> %.0f addresses\n", std::pow(2.0, 24));
    const std::vector<Window> m2 = {{"RAM", 0x0000, 0x1000}, {"LEDS", 0x2000, 4}, {"TIMER", 0x3000, 0x10}, {"DMA", 0x4000, 0x20}};
    for (unsigned a : {0x0FFFu, 0x2004u, 0x401Fu, 0x4020u}) decode(m2, a);
    std::printf("M3: 32-bit bus at 125 MHz: peak %.0f B/s; address+data cycles: %.0f B/s; CPU share with DMA on 3 of 10 cycles: %d %%; CPU bytes/s %.0f\n",
                4 * 125e6, 4 * 125e6 / 2, 70, 4 * 125e6 / 2 * 0.7);
    std::printf("M5: STATUS 0x0B handled bit 3 -> write 0x%02X; CTRL 0x40000010 set bit 2 -> 0x%08X; then clear bit 4 -> 0x%08X; LSR of UART at 0x3e8 = 0x%03X\n",
                1u << 3, 0x40000010u | (1u << 2), (0x40000010u | (1u << 2)) & ~(1u << 4), 0x3e8 + 5);
    std::printf("M7: byte every 20 ticks, 1-byte receiver: polling threshold 20 ticks; 180-tick pass: %d arrivals, %d lost per long pass; ring >= %d (+1 boundary = %d), choose 16; 65536 bytes x 2 ticks = %d ticks in handlers\n",
                180 / 20, 180 / 20 - 1, 180 / 20, 180 / 20 + 1, 65536 * 2);
    {
        const unsigned irr = 0xA4, imr = 0x04, isr = 0x40;
        const unsigned cand = irr & ~imr;
        int best = -1, inservice = -1;
        for (int l = 0; l < 8; ++l) { if ((isr >> l) & 1) { inservice = l; break; } if ((cand >> l) & 1) { best = l; break; } }
        std::printf("M9: IRR 0x%02X IMR 0x%02X ISR 0x%02X: pending&~mask 0x%02X, delivered line %d (vector %d), first in-service line %d; imr 0x3A masks lines:",
                    irr, imr, isr, cand, best, 32 + best, inservice);
        for (int l = 0; l < 8; ++l) if ((0x3A >> l) & 1) std::printf(" %d", l);
        std::printf("\n");
    }
    for (int n : {6, 8, 1024}) std::printf("M11: n=%d words: PIO %d ticks, DMA %d ticks, saving %d (%.1f %%)\n", n, n, 8, n - 8, 100.0 * (n - 8) / n);
    std::printf("M11: buffer at 24 length 40, 16-byte lines: lines %d to %d\n", 24 / 16, (24 + 40 - 1) / 16);

    std::printf("== Final ==\n");
    std::printf("F1: 18 address lines -> %.0f\n", std::pow(2.0, 18));
    const std::vector<Window> f2 = {{"A", 0x1000, 0x400}, {"B", 0x1400, 0x100}, {"C", 0x1480, 0x80}};
    overlaps(f2);
    std::printf("F2: open-drain AND of 0x3C and 0x55 = 0x%02X\n", 0x3C & 0x55);
    std::printf("F3: UART at 0x2e8: LSR 0x%03X, IER 0x%03X; STATUS 0x05 handled bit 2 -> write 0x%02X; CTRL 0x80000000 set bit 1 -> 0x%08X\n",
                0x2e8 + 5, 0x2e8 + 1, 1u << 2, 0x80000000u | 2u);
    std::printf("F5: 16-byte FIFO, byte per 10 ticks: polling threshold %d ticks; 350-tick pass: %d arrivals (+1 = %d) -> 64 slots; handler 2 ticks x 65536 = %d of %d ticks = %.0f %%\n",
                16 * 10, 350 / 10, 350 / 10 + 1, 65536 * 2, 65536 * 10, 100.0 * 2 / 10);
    {
        const unsigned irr = 0x1A, imr = 0x08, isr = 0x02;
        const unsigned cand = irr & ~imr;
        int best = -1;
        for (int l = 0; l < 8; ++l) { if ((isr >> l) & 1) break; if ((cand >> l) & 1) { best = l; break; } }
        std::printf("F7: IRR 0x%02X IMR 0x%02X ISR 0x%02X: pending&~mask 0x%02X; delivered: %s\n", irr, imr, isr, cand,
                    best < 0 ? "none (line 1 in service blocks everything below it)" : "a line");
    }
    for (int n : {5, 2048}) std::printf("F9: n=%d: DMA saves %d ticks (%.1f %%)\n", n, n - 8, 100.0 * (n - 8) / n);
    std::printf("F9: buffer at 40 length 24: lines %d to %d\n", 40 / 16, (40 + 24 - 1) / 16);
    {
        std::uint64_t s;
        unsigned f = bar_size(0xFFF00008u, false, false, 0, s); std::printf("F12: mem32 0xFFF00008: flags 0x%X size 0x%llX (%llu bytes)\n", f, (unsigned long long)s, (unsigned long long)s);
        f = bar_size(0xFFFFFF81u, true, false, 0, s); std::printf("F12: io 0xFFFFFF81: flags 0x%X size 0x%llX (%llu bytes)\n", f, (unsigned long long)s, (unsigned long long)s);
        f = bar_size(0xFFFE0004u, false, true, 0xFFFFFFFFu, s); std::printf("F12: mem64 0xFFFE0004/0xFFFFFFFF: flags 0x%X size 0x%llX (%llu bytes)\n", f, (unsigned long long)s, (unsigned long long)s);
        std::printf("F12: command 0x0403 = I/O decode 0x1 + memory decode 0x2 + INTx disable 0x400; bus master bit (0x4) clear\n");
        std::printf("F13: buggy ~0xFFFFC008 + 1 = 0x%X; correct (mask 0xF first) 0x%X\n", (~0xFFFFC008u + 1u), (~(0xFFFFC008u & ~0xFu) + 1u));
    }
    std::printf("F15: SETUP 80 06 00 03 09 04 FF 00: bmRequestType 0x80 IN standard device; bRequest 6 GET_DESCRIPTOR; wValue 0x%04X type %d index %d; wIndex 0x%04X; wLength %d\n",
                0x0300, 3, 0, 0x0409, 0xFF);
    std::printf("F15: endpoint 07 05 83 03 40 00 0A: EP %d %s, type %d (interrupt), max packet %d, bInterval %d\n", 0x83 & 0xF, (0x83 & 0x80) ? "IN" : "OUT", 3, 0x0040, 10);
    std::printf("F17: 0x93 -> addr 0x%02X %s; 0xD0 -> addr 0x%02X %s; datasheet 0xA0/0xA1 -> 7-bit 0x%02X\n", 0x93 >> 1, (0x93 & 1) ? "read" : "write", 0xD0 >> 1, (0xD0 & 1) ? "read" : "write", 0xA0 >> 1);
    can({{"node X", 0x2A0}, {"node Y", 0x29F}, {"node Z", 0x2A1}});
    std::printf("F17: 57600 baud 8N1 (10 bit times) -> %d bytes/s; 8E2 (12 bit times) -> %d bytes/s\n", 57600 / 10, 57600 / 12);
    std::printf("F19:\n");
    timer(8e6, 1000, 1); timer(8e6, 1000, 64); timer(8e6, 2000, 1);
    {
        const double actual = 8e6 / 8001;
        std::printf("F19: reload 8000 instead of 7999 -> %.4f Hz, error %+.1f ppm, %.2f s per day\n", actual, (actual - 1000) / 1000 * 1e6, 86400.0 * (1000 - actual) / 1000);
        std::printf("F19: 2^32 us = %.1f s = %.2f min; 2^16 ms = %.3f s\n", std::pow(2.0, 32) / 1e6, std::pow(2.0, 32) / 60e6, 65536 / 1000.0);
    }
    {
        const std::uint32_t start = 0xFFFFFFF0u, deadline = start + 40u;
        for (std::uint32_t now : {start, static_cast<std::uint32_t>(start + 10u), deadline, static_cast<std::uint32_t>(deadline + 10u)})
            std::printf("F20: now 0x%08X (%u) deadline 0x%08X (%u): naive %s, wrap-safe diff %d -> %s\n", now, now, deadline, deadline,
                        now >= deadline ? "yes" : "no", static_cast<std::int32_t>(now - deadline),
                        static_cast<std::int32_t>(now - deadline) >= 0 ? "yes" : "no");
        std::printf("F20: wrap-safe limit 2^31 ms = %.2f days\n", std::pow(2.0, 31) / 86400e3);
    }
    std::printf("F22 (forensic): 60-tick pass at 10 ticks/byte: %d arrivals, FIFO 4 keeps 4, loses %d; polling threshold with a 4-byte FIFO = %d ticks; 5461 long passes x 2 = %d\n",
                60 / 10, 60 / 10 - 4, 4 * 10, 5461 * 2);
    std::printf("F23 (design): 200 Hz from 16 MHz, 16-bit timer:\n");
    timer(16e6, 200, 1); timer(16e6, 200, 8);
    std::printf("F23: I2C 100 kbit/s, 6-byte register read: ~%d bit times = %.2f ms; UART 115200, 22 chars x 10 bits = %.2f ms per reading; period %.1f ms\n",
                1 + 9 + 9 + 1 + 9 + 6 * 9 + 1, (1 + 9 + 9 + 1 + 9 + 6 * 9 + 1) / 100e3 * 1e3, 22 * 10 / 115200.0 * 1e3, 1000.0 / 200);
    std::printf("F23: longest main-loop pass 25 ms at 5 ms per reading -> %d readings buffered (+1) -> 8 slots\n", 25 / 5);
    std::printf("== Practical ==\n");
    std::printf("P: first address byte 0x3C -> 7-bit 0x%02X, %s; 0xB4 -> 0x%02X write; 0xB5 -> 0x%02X read; datasheet 'B4/B5' -> configure 0x%02X\n",
                0x3C >> 1, (0x3C & 1) ? "read" : "write", 0xB4 >> 1, 0xB5 >> 1, 0xB4 >> 1);
    std::printf("P: capture 349 samples at 4 per bit = %.2f bit times; 3 x START + 1 repeated START, 3 x STOP, %d bytes x 9 clocks = %d clocks\n", 349 / 4.0, 9, 9 * 9);

    // Addendum (exam pass of 2026-10-25): further numbers quoted in the keys.
    std::printf("== Addendum ==\n");
    {
        const unsigned base = 24, len = 40, line = 16;
        for (unsigned l = base / line; l <= (base + len - 1) / line; ++l) {
            const unsigned lo = l * line, hi = lo + line - 1;
            const bool shared = lo < base || hi > base + len - 1;
            std::printf("M11: line %u = bytes %u-%u %s\n", l, lo, hi, shared ? "(shared with other data)" : "(buffer only)");
        }
        std::printf("M11: DMA break-even: PIO n ticks vs DMA 8 ticks -> DMA cheaper from n = %d\n", 8 + 1);
    }
    {
        std::printf("M10: line 3 handler forgets EOI -> lines still served:");
        for (int l = 0; l < 8; ++l) if (l < 3) std::printf(" %d", l);
        std::printf("; lines blocked:");
        for (int l = 0; l < 8; ++l) if (l >= 3) std::printf(" %d", l);
        std::printf("\n");
    }
    {
        // F7 after line 1's EOI: its new request (IRR bit 1) is still pending
        std::printf("F7: after EOI of line 1 (ISR 0x00), IRR 0x1A IMR 0x08: candidates 0x%02X -> line 1 (vector %d) first, then line 4 (vector %d); line 3 stays masked\n",
                    0x1Au & ~0x08u, 32 + 1, 32 + 4);
    }
    {
        std::printf("F22: the first long gap: polls at 60 and 120; arrivals at");
        int kept = 0;
        for (int t = 70; t <= 120; t += 10) std::printf(" %d", t);
        std::printf("; the 4-byte FIFO keeps");
        for (int t = 70; t <= 120; t += 10) if (kept < 4) { std::printf(" %d", t); ++kept; }
        std::printf("; lost 110 120 (arrival at 120 is processed before the poll at 120)\n");
        std::printf("F22: ring needed >= %d bytes per long pass (+1 boundary = %d) -> 8 slots; received %d = 65536 - %d\n",
                    60 / 10, 60 / 10 + 1, 65536 - 10922, 10922);
        std::printf("F22: passes 81923 + 5461 = %d; long passes are 1 in %d\n", 81923 + 5461, (81923 + 5461) / 5461);
    }
    {
        std::printf("P: address bytes in binary: 0x3C = ");
        for (int b = 7; b >= 0; --b) std::printf("%d", (0x3C >> b) & 1);
        std::printf(", 0xB4 = ");
        for (int b = 7; b >= 0; --b) std::printf("%d", (0xB4 >> b) & 1);
        std::printf(", 0xB5 = ");
        for (int b = 7; b >= 0; --b) std::printf("%d", (0xB5 >> b) & 1);
        std::printf("; data 0x2C = ");
        for (int b = 7; b >= 0; --b) std::printf("%d", (0x2C >> b) & 1);
        std::printf(", 0x8F = ");
        for (int b = 7; b >= 0; --b) std::printf("%d", (0x8F >> b) & 1);
        std::printf("\n");
    }
    {
        std::printf("F12: 0x100000 bytes = %d KiB = %d MiB; 0x20000 bytes = %d KiB\n", 0x100000 / 1024, 0x100000 / 1048576, 0x20000 / 1024);
        std::printf("F19: 16-bit ms counter wraps after %d ms; 2^31 ms wrap-safe limit = %.0f ms\n", 65536, std::pow(2.0, 31));
        const unsigned wire = (0xB4u << 1) & 0xFFu;   // as probe()/read_reg() of the model would send it
        std::printf("P: 0xB4 configured as a 7-bit address -> 0xB4 << 1 = 0x%03X, byte on the wire 0x%02X = 7-bit 0x%02X %s -> NACK (no target there)\n",
                    0xB4u << 1, wire, wire >> 1, (wire & 1) ? "R" : "W");
        std::printf("F23: UART line 1.91 ms of a 5.0 ms period = %.1f %%; I2C read 0.84 ms = %.1f %% of the period\n",
                    22 * 10 / 115200.0 * 1e3 / 5.0 * 100, 0.84 / 5.0 * 100);
    }
    return 0;
}

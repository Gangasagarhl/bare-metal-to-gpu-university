// iotest.cc - DR301: the random-I/O test loop (see iotest.h).
#include "iotest.h"
#include "kbase.h"
#include "../F4-03/irq.h"

namespace iotest {
namespace {
struct Slot { bool busy, write; uint32_t block, gen; };
Slot g_slot[32];

bool check(const uint8_t* buf, uint32_t block, uint32_t gen)
{
    if (gen == 0) {                                  // never written: the image is all zero
        for (int i = 0; i < 4096; ++i) if (buf[i]) return false;
        return true;
    }
    alignas(4) static uint8_t want[4096];
    fill(want, block, gen);
    return memcmp(buf, want, 4096) == 0;
}
}  // namespace

Result run(const Disk& d, uint32_t ops, int depth, uint32_t seed, uint8_t* gen)
{
    Result r{};
    int inflight = 0;
    uint32_t x = seed;
    const uint64_t t0 = timer::ms();
    auto complete_one = [&]() {
        bool ok = false;
        const int s = d.wait_any(ok);
        Slot& sl = g_slot[s];
        if (!ok) ++r.errors;
        else if (!sl.write && !check(d.buffer(s), sl.block, sl.gen)) {
            if (r.bad_reads < 3) kprintf("  BAD READ block %u (expected generation %u)\n", sl.block, sl.gen);
            ++r.bad_reads;
        }
        sl.busy = false;
        --inflight;
    };
    for (uint32_t n = 0; n < ops; ++n) {
        const uint32_t block = xorshift32(x) % kBlocks;
        const bool write = xorshift32(x) & 1;
        // Requests in flight may complete in any order, so never have two on one block.
        for (int s = 0; s < d.slots; ++s)
            if (g_slot[s].busy && g_slot[s].block == block) { while (inflight) complete_one(); break; }
        if (inflight == depth) complete_one();
        int s = 0;
        while (g_slot[s].busy) ++s;
        Slot& sl = g_slot[s];
        sl = Slot{true, write, block, gen[block]};
        if (write) {
            sl.gen = gen[block] = static_cast<uint8_t>(gen[block] % 255 + 1);   // 1..255
            fill(d.buffer(s), block, sl.gen);
            ++r.writes;
        } else {
            ++r.reads;
        }
        d.start(s, write, block);
        ++inflight;
    }
    while (inflight) complete_one();
    r.ms = timer::ms() - t0;
    return r;
}
}  // namespace iotest

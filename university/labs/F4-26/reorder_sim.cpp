// reorder_sim.cpp - F4-26 forensic lab: the university's own MODEL of a CPU whose stores can
// become visible out of order, and of a DMA device that reads a descriptor ring. It is not
// a real CPU trace: it shows the mechanism with numbers we can check. Three CPU models:
//   in-order   every store becomes visible in program order (like x86-64's TSO for stores)
//   weak       stores to different addresses may become visible in any order
//   weak+fence weak, but the driver puts a release fence before publishing the ring index
// The driver submits 1000 requests through 4 descriptor slots. For each request it writes
// the descriptor (address and length), the ring entry, then the ring index (the device polls
// the index, as a virtio device does after a doorbell).
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

namespace {

enum Field { kAddr, kLen, kRing, kIdx };
const char* const kFieldName[] = {"addr", "len", "ring", "idx"};

struct Store {
    Field field;
    int slot;            // descriptor slot (or ring position)
    uint64_t value;
    int epoch;           // stores of a later epoch may not pass stores of an earlier one
};

struct Memory {          // what the device can see
    uint64_t addr[4] = {}, len[4] = {}, ring[4] = {}, idx = 0;
    void apply(const Store& s)
    {
        switch (s.field) {
        case kAddr: addr[s.slot] = s.value; break;
        case kLen: len[s.slot] = s.value; break;
        case kRing: ring[s.slot] = s.value; break;
        case kIdx: idx = s.value; break;
        }
    }
};

struct Result {
    int requests = 0, stale = 0, max_pending = 0;
};

Result run(const char* model, bool weak, bool fence, bool trace)
{
    std::mt19937 rng(402);
    Memory mem;
    std::vector<Store> buffer;   // the CPU's store buffer: issued, not yet visible
    Result r;
    uint64_t next_idx = 0, seen_idx = 0;
    int epoch = 0;
    long t = 0;
    int traced = 0;
    auto expect_addr = [](uint64_t req) { return 0x40200000 + req * 0x1000; };
    auto expect_len = [](uint64_t req) { return 512 + (req % 8) * 512; };
    while (r.requests < 1000) {
        ++t;
        // CPU: issue the next request's stores when the buffer has room
        if (buffer.size() < 6 && next_idx == seen_idx) {
            uint64_t req = next_idx;
            int slot = static_cast<int>(req % 4);
            buffer.push_back({kAddr, slot, expect_addr(req), epoch});
            buffer.push_back({kLen, slot, expect_len(req), epoch});
            buffer.push_back({kRing, slot, static_cast<uint64_t>(slot), epoch});
            if (fence) {
                ++epoch;     // release fence: everything above becomes visible first
            }
            buffer.push_back({kIdx, 0, req + 1, epoch});
            ++next_idx;
            if (trace && traced < 40) {
                std::printf("t=%05ld cpu  issue  req %llu: desc[%d].addr=0x%llx desc[%d].len=%llu ring[%d]=%d idx=%llu%s\n",
                            t, static_cast<unsigned long long>(req), slot,
                            static_cast<unsigned long long>(expect_addr(req)), slot,
                            static_cast<unsigned long long>(expect_len(req)), slot, slot,
                            static_cast<unsigned long long>(req + 1), fence ? " (fence before idx)" : "");
                ++traced;
            }
        }
        r.max_pending = std::max(r.max_pending, static_cast<int>(buffer.size()));
        // Store buffer: one store becomes visible per step
        if (!buffer.empty()) {
            size_t pick = 0;
            if (weak) {
                // a store may become visible if it is in the oldest epoch and no older store to
                // the SAME location is still waiting (every CPU keeps one location's order)
                int oldest = buffer[0].epoch;
                std::vector<size_t> allowed;
                for (size_t i = 0; i < buffer.size(); ++i) {
                    bool older_same = false;
                    for (size_t j = 0; j < i; ++j) {
                        older_same = older_same || (buffer[j].field == buffer[i].field && buffer[j].slot == buffer[i].slot);
                    }
                    if (buffer[i].epoch == oldest && !older_same) {
                        allowed.push_back(i);
                    }
                }
                pick = allowed[std::uniform_int_distribution<size_t>(0, allowed.size() - 1)(rng)];
            }
            Store s = buffer[pick];
            buffer.erase(buffer.begin() + static_cast<long>(pick));
            mem.apply(s);
            if (trace && traced < 40) {
                if (s.field == kIdx) {
                    std::printf("t=%05ld mem  visible idx = %llu\n", t, static_cast<unsigned long long>(s.value));
                } else {
                    std::printf("t=%05ld mem  visible %s[%d] = 0x%llx\n", t, kFieldName[s.field], s.slot,
                                static_cast<unsigned long long>(s.value));
                }
                ++traced;
            }
        }
        // Device: polls the index; for each new entry reads ring and descriptor from memory
        while (seen_idx < mem.idx) {
            uint64_t req = seen_idx;
            int pos = static_cast<int>(req % 4);
            uint64_t slot = mem.ring[pos];
            uint64_t a = mem.addr[slot % 4], l = mem.len[slot % 4];
            bool ok = slot == static_cast<uint64_t>(pos) && a == expect_addr(req) && l == expect_len(req);
            if (!ok) {
                ++r.stale;
            }
            if (trace && (traced < 40 || (!ok && r.stale <= 3))) {
                std::printf("t=%05ld dev  req %llu: idx=%llu ring[%d]=%llu -> DMA to 0x%llx, %llu bytes%s\n", t,
                            static_cast<unsigned long long>(req), static_cast<unsigned long long>(mem.idx), pos,
                            static_cast<unsigned long long>(slot), static_cast<unsigned long long>(a),
                            static_cast<unsigned long long>(l), ok ? "" : "   <-- not what the driver wrote for this request");
                ++traced;
            }
            ++seen_idx;
            ++r.requests;
        }
    }
    std::printf("%-10s: %d requests, %d used a stale or wrong descriptor; up to %d stores waiting\n", model,
                r.requests, r.stale, r.max_pending);
    return r;
}

} // namespace

int main()
{
    std::printf("MODEL, not a CPU trace: store-visibility order and a polling DMA device (seed 402)\n");
    std::printf("== trace of the weak model without a fence (first 40 events, then the first 3 bad requests) ==\n");
    run("weak", true, false, true);
    std::printf("== summary ==\n");
    Result a = run("in-order", false, false, false);
    Result b = run("weak", true, false, false);
    Result c = run("weak+fence", true, true, false);
    bool expected = a.stale == 0 && b.stale > 0 && c.stale == 0;
    std::printf("%s\n", expected ? "as expected: only the weak model without a fence corrupts requests"
                                 : "UNEXPECTED summary");
    return expected ? 0 : 1;
}

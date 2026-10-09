// publish.cc - F4-26: three ways to publish a filled descriptor by writing the ring index.
// Compiled (not run) for x86-64, AArch64 and RISC-V by run.sh to compare the instructions.
#include <atomic>
#include <cstdint>

struct Desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags, next;
};
struct Ring {
    Desc desc[8];
    uint16_t ring[8];
    uint16_t idx;
};

// 1. plain stores: nothing orders the descriptor before the index
void publish_plain(Ring* r, uint64_t addr, uint32_t len)
{
    r->desc[0].addr = addr;
    r->desc[0].len = len;
    r->ring[r->idx % 8] = 0;
    r->idx = static_cast<uint16_t>(r->idx + 1);
}

// 2. a release fence before the index store
void publish_fence(Ring* r, uint64_t addr, uint32_t len)
{
    r->desc[0].addr = addr;
    r->desc[0].len = len;
    r->ring[r->idx % 8] = 0;
    std::atomic_thread_fence(std::memory_order_release);
    r->idx = static_cast<uint16_t>(r->idx + 1);
}

// 3. the index itself stored with release semantics
void publish_release(Ring* r, uint64_t addr, uint32_t len)
{
    r->desc[0].addr = addr;
    r->desc[0].len = len;
    uint16_t i = r->idx;
    r->ring[i % 8] = 0;
    std::atomic_ref<uint16_t>(r->idx).store(static_cast<uint16_t>(i + 1), std::memory_order_release);
}

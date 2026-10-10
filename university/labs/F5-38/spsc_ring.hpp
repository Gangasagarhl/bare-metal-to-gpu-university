// spsc_ring.hpp - DS401 F5-38: a single-producer single-consumer ring of fixed-size slots
// in memory shared by two processes. The producer writes a slot, then publishes it by
// advancing `head` (release); the consumer polls `head` (acquire), reads the slot, then
// frees it by advancing `tail`. No system call and no kernel on the data path.
#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <stdexcept>
#include <sys/mman.h>

constexpr size_t kSlots = 1024;      // a power of two
constexpr size_t kSlotBytes = 64;

struct Ring
{
    alignas(64) std::atomic<uint64_t> head{0};   // next slot the producer will fill
    alignas(64) std::atomic<uint64_t> tail{0};   // next slot the consumer will read
    alignas(64) char slots[kSlots][kSlotBytes];
};
static_assert(std::atomic<uint64_t>::is_always_lock_free, "the ring needs lock-free 64-bit atomics");

// Map a ring that stays shared with child processes created later by fork().
inline Ring* mapSharedRing()
{
    void* p = ::mmap(nullptr, sizeof(Ring), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }
    return new (p) Ring();
}

inline void unmapRing(Ring* r)
{
    r->~Ring();
    ::munmap(r, sizeof(Ring));
}

inline bool tryPush(Ring& r, const void* msg)
{
    const uint64_t h = r.head.load(std::memory_order_relaxed);
    if (h - r.tail.load(std::memory_order_acquire) == kSlots) {
        return false;                                       // full
    }
    std::memcpy(r.slots[h % kSlots], msg, kSlotBytes);
    r.head.store(h + 1, std::memory_order_release);         // publish
    return true;
}

inline bool tryPop(Ring& r, void* msg)
{
    const uint64_t t = r.tail.load(std::memory_order_relaxed);
    if (r.head.load(std::memory_order_acquire) == t) {
        return false;                                       // empty
    }
    std::memcpy(msg, r.slots[t % kSlots], kSlotBytes);
    r.tail.store(t + 1, std::memory_order_release);         // free the slot
    return true;
}

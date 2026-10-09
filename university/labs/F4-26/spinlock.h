// spinlock.h - F4-26: a ticket lock written only with std::atomic, so the same file is
// correct on x86-64, AArch64 and RISC-V: the memory orders say what must be ordered and the
// compiler emits each CPU's own instructions (F4-30 compares them).
#pragma once
#include <atomic>
#include <cstdint>
#include "arch.h"

class TicketLock {
public:
    void lock()
    {
        uint32_t me = next_.fetch_add(1, std::memory_order_relaxed);
        while (serving_.load(std::memory_order_acquire) != me) {   // acquire: see the previous
            arch::spin_wait();                                        // owner's writes
        }
    }
    void unlock()
    {
        serving_.store(serving_.load(std::memory_order_relaxed) + 1,
                       std::memory_order_release);                    // release: publish ours
        arch::spin_wake();                                            // wake waiters (if the CPU sleeps)
    }

private:
    std::atomic<uint32_t> next_{0};
    std::atomic<uint32_t> serving_{0};
};

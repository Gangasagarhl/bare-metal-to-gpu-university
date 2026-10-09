// ring.h - DR301 F4-03: a single-producer, single-consumer ring buffer.
// One side runs in an interrupt handler, the other in normal code. Each index is written
// by only one side, so no lock is needed on one CPU; 'volatile' stops the compiler from
// caching an index the other side changes. N must be a power of two.
#pragma once
#include <stddef.h>
#include <stdint.h>

template <size_t N>
class Ring {
    static_assert((N & (N - 1)) == 0, "N must be a power of two");
public:
    bool push(uint8_t b)                       // producer side
    {
        if (head_ - tail_ == N) { dropped_ = dropped_ + 1; return false; }   // full: count the loss
        buf_[head_ % N] = b;
        asm volatile("" : : : "memory");       // store the byte before publishing it
        head_ = head_ + 1;
        return true;
    }
    bool pop(uint8_t& b)                       // consumer side
    {
        if (head_ == tail_) return false;      // empty
        b = buf_[tail_ % N];
        asm volatile("" : : : "memory");
        tail_ = tail_ + 1;
        return true;
    }
    size_t size() const { return head_ - tail_; }
    uint32_t dropped() const { return dropped_; }
    uint32_t high_water() const { return high_; }
    void note_level() { if (size() > high_) high_ = static_cast<uint32_t>(size()); }

private:
    uint8_t buf_[N];
    volatile size_t head_ = 0, tail_ = 0;      // free-running counters; full when they differ by N
    volatile uint32_t dropped_ = 0;
    uint32_t high_ = 0;
};

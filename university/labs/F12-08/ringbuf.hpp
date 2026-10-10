// ringbuf.hpp - a fixed-capacity ring buffer of ints, as a driver or a sensor task keeps one.
// push refuses when full; pop returns nothing when empty. This version has a planted bug.
#pragma once
#include <array>
#include <cstddef>
#include <optional>

class RingBuffer {
public:
    static constexpr std::size_t kCapacity = 4;

    bool push(int v)
    {
        if (count_ == kCapacity) {
            return false;
        }
        slots_[head_] = v;
        head_ = (head_ + 1) % kCapacity;
        ++count_;
        return true;
    }

    std::optional<int> pop()
    {
        if (count_ == 0) {
            return std::nullopt;
        }
        const int v = slots_[tail_];
        if (++tail_ == kCapacity) {
            tail_ = 1;  // planted bug: the wrap-around should go back to slot 0
        }
        --count_;
        return v;
    }

private:
    std::array<int, kCapacity> slots_{};
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t count_ = 0;
};

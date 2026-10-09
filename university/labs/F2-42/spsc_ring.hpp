// spsc_ring.hpp (F2-42): a lock-free ring buffer for exactly ONE producer thread and ONE
// consumer thread. head_ is written only by the consumer, tail_ only by the producer.
#pragma once
#include <atomic>
#include <cstddef>
#include <optional>
#include <vector>

template <typename T>
class SpscRing
{
public:
    explicit SpscRing(std::size_t capacityPowerOfTwo) : slots_(capacityPowerOfTwo), mask_(capacityPowerOfTwo - 1) {}

    bool tryPush(const T& value)  // producer thread only
    {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);   // our own variable
        const std::size_t head = head_.load(std::memory_order_acquire);   // consumer's progress
        if (tail - head == slots_.size()) {
            return false;  // full
        }
        slots_[tail & mask_] = value;                        // (1) write the slot
        tail_.store(tail + 1, std::memory_order_release);    // (2) publish it
        return true;
    }

    std::optional<T> tryPop()  // consumer thread only
    {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);   // sees (2), so also (1)
        if (head == tail) {
            return std::nullopt;  // empty
        }
        T value = slots_[head & mask_];
        head_.store(head + 1, std::memory_order_release);    // give the slot back
        return value;
    }

private:
    std::vector<T> slots_;
    const std::size_t mask_;
    alignas(64) std::atomic<std::size_t> head_{0};  // next slot to read
    alignas(64) std::atomic<std::size_t> tail_{0};  // next slot to write
};

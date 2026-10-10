// flightrec.h - F9-70: a flight recorder for a real-time loop.
// A fixed-size ring of small binary records in memory. The real-time thread only copies a
// record into the ring: no allocation, no lock, no system call, no formatting. When the ring is
// full the oldest record is overwritten, so the recorder always holds the LAST N records, which
// is what you want after an incident. Formatting happens later, outside the loop.
// One writer thread; read it after the writer has stopped (a reader running at the same time
// would need a sequence counter per record; see the lab extension).
#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

enum class Ev : uint16_t { Cycle = 1, Late = 2, Marker = 3 };

struct Record {
    int64_t t_ns;        // CLOCK_MONOTONIC time of the event
    uint32_t cycle;      // loop iteration number
    int32_t value;       // event-specific: lateness in microseconds for Cycle and Late
    Ev event;
};

template <std::size_t N>
class FlightRecorder {
public:
    void push(const Record& r) noexcept
    {
        uint64_t h = head_.load(std::memory_order_relaxed);
        ring_[h % N] = r;
        head_.store(h + 1, std::memory_order_release);
    }
    uint64_t written() const noexcept { return head_.load(std::memory_order_acquire); }
    uint64_t overwritten() const noexcept { return written() > N ? written() - N : 0; }
    // Visit the records still held, oldest first.
    template <typename F>
    void for_each(F&& f) const
    {
        uint64_t h = written();
        for (uint64_t i = h > N ? h - N : 0; i < h; ++i) {
            f(ring_[i % N]);
        }
    }

private:
    std::array<Record, N> ring_{};
    std::atomic<uint64_t> head_{0};
};

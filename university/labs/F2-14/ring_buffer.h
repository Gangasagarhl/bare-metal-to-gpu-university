#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <array>
#include <cstddef>

// A fixed-size first-in, first-out queue for N values of type T, stored in a std::array.
// Invariant: count_ <= N and head_ < N. The oldest value is at data_[head_].
template <typename T, std::size_t N>
class RingBuffer
{
    static_assert(N > 0, "RingBuffer needs room for at least one value");

public:
    // Adds a value at the back. Returns false (and changes nothing) when full.
    bool push(const T& value)
    {
        if (count_ == N) {
            return false;
        }
        data_[(head_ + count_) % N] = value;
        count_ = count_ + 1;
        return true;
    }

    // Removes the oldest value into out. Returns false (and changes nothing) when empty.
    bool pop(T& out)
    {
        if (count_ == 0) {
            return false;
        }
        out = data_[head_];
        head_ = (head_ + 1) % N;
        count_ = count_ - 1;
        return true;
    }

    std::size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == N; }
    static constexpr std::size_t capacity() { return N; }

private:
    std::array<T, N> data_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
};

#endif

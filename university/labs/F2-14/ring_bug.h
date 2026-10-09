#ifndef RING_BUG_H
#define RING_BUG_H

#include <array>
#include <cstddef>

// Forensic evidence: the order rail's buffer, "simplified" last month.
template <typename T, std::size_t N>
class OrderRail
{
public:
    bool push(const T& value)
    {
        if (count_ == N) {
            return false;
        }
        data_[count_ % N] = value;
        count_ = count_ + 1;
        return true;
    }

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

private:
    std::array<T, N> data_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
};

#endif

// bounded_queue.hpp (F2-37): a bounded blocking queue built from one mutex and two
// condition variables. push() waits while the queue is full; pop() waits while it is empty.
// close() wakes everybody: pushes are refused and pop() returns std::nullopt once drained.
#pragma once
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <vector>

template <typename T>
class BoundedQueue
{
public:
    explicit BoundedQueue(std::size_t capacity) : slots_(capacity) {}

    // Returns false if the queue was closed.
    bool push(T value)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (count_ == slots_.size() && !closed_) {
            ++pushWaits_;  // statistics only: how often a producer had to wait
        }
        notFull_.wait(lock, [this] { return count_ < slots_.size() || closed_; });
        if (closed_) {
            return false;
        }
        slots_[(head_ + count_) % slots_.size()] = std::move(value);
        ++count_;
        lock.unlock();         // unlock first, so the woken consumer can take the lock at once
        notEmpty_.notify_one();
        return true;
    }

    // Returns std::nullopt when the queue is closed and empty.
    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (count_ == 0 && !closed_) {
            ++popWaits_;
        }
        notEmpty_.wait(lock, [this] { return count_ > 0 || closed_; });
        if (count_ == 0) {
            return std::nullopt;  // closed and drained
        }
        T value = std::move(slots_[head_]);
        head_ = (head_ + 1) % slots_.size();
        --count_;
        lock.unlock();
        notFull_.notify_one();
        return value;
    }

    void close()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        notFull_.notify_all();
        notEmpty_.notify_all();
    }

    // Statistics, read after all threads have finished.
    long pushWaits() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pushWaits_;
    }

    long popWaits() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return popWaits_;
    }

private:
    mutable std::mutex mutex_;          // protects everything below
    std::condition_variable notFull_;   // signalled when a slot becomes free
    std::condition_variable notEmpty_;  // signalled when an item arrives
    std::vector<T> slots_;              // ring buffer storage
    std::size_t head_ = 0;              // index of the oldest item
    std::size_t count_ = 0;             // number of items stored
    bool closed_ = false;
    long pushWaits_ = 0;
    long popWaits_ = 0;
};

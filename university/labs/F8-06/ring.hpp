// F8-06 ring.hpp: the ring all-reduce of F8-05 Listing 1, copied unchanged (the mailbox,
// the chunk ranges and ringAllReduce), so that the training step can use it.
#pragma once
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

template <typename T>
class Mailbox   // one producer (the left neighbour), one consumer (the owner)
{
public:
    void put(std::vector<T> chunk)
    {
        std::unique_lock<std::mutex> lock(m_);
        notFull_.wait(lock, [&] { return q_.size() < capacity_; });   // flow control
        q_.push_back(std::move(chunk));
        notEmpty_.notify_one();
    }
    std::vector<T> take()
    {
        std::unique_lock<std::mutex> lock(m_);
        notEmpty_.wait(lock, [&] { return !q_.empty(); });
        std::vector<T> chunk = std::move(q_.front());
        q_.pop_front();
        notFull_.notify_one();
        return chunk;
    }

private:
    std::mutex m_;
    std::condition_variable notFull_, notEmpty_;
    std::deque<std::vector<T>> q_;
    const std::size_t capacity_ = 2;
};

struct Range
{
    std::size_t begin;
    std::size_t end;
};

// chunk k of segment g: the same number of chunks in every segment (some may be empty)
inline Range chunkRange(std::size_t count, std::size_t n, std::size_t g, std::size_t k, std::size_t chunks)
{
    const std::size_t s0 = g * count / n;
    const std::size_t len = (g + 1) * count / n - s0;
    return {s0 + k * len / chunks, s0 + (k + 1) * len / chunks};
}

template <typename T>
std::size_t ringAllReduce(std::vector<std::vector<T>>& buf, std::size_t chunkElems)
{
    const std::size_t n = buf.size();
    const std::size_t count = buf[0].size();
    if (n == 1) {
        return 0;
    }
    const std::size_t maxSeg = (count + n - 1) / n;
    const std::size_t chunks = maxSeg == 0 ? 1 : (maxSeg + chunkElems - 1) / chunkElems;
    std::vector<Mailbox<T>> inbox(n);
    std::vector<std::size_t> sentElems(n, 0);
    auto rank = [&](std::size_t r) {
        std::vector<T>& mine = buf[r];
        Mailbox<T>& next = inbox[(r + 1) % n];
        Mailbox<T>& in = inbox[r];
        for (int phase = 0; phase < 2; ++phase) {           // 0: reduce-scatter, 1: all-gather
            for (std::size_t step = 0; step + 1 < n; ++step) {
                const std::size_t sendSeg = (r + n * 2 - step + phase) % n;
                const std::size_t recvSeg = (r + n * 2 - step - 1 + phase) % n;
                for (std::size_t k = 0; k < chunks; ++k) {
                    const Range s = chunkRange(count, n, sendSeg, k, chunks);
                    next.put(std::vector<T>(mine.begin() + static_cast<long>(s.begin),
                                            mine.begin() + static_cast<long>(s.end)));
                    sentElems[r] += s.end - s.begin;
                    const Range d = chunkRange(count, n, recvSeg, k, chunks);
                    const std::vector<T> got = in.take();
                    for (std::size_t i = 0; i < got.size(); ++i) {
                        if (phase == 0) {
                            mine[d.begin + i] += got[i];     // reduce
                        } else {
                            mine[d.begin + i] = got[i];      // gather
                        }
                    }
                }
            }
        }
    };
    std::vector<std::thread> threads;
    for (std::size_t r = 0; r < n; ++r) {
        threads.emplace_back(rank, r);
    }
    for (auto& t : threads) {
        t.join();
    }
    std::size_t most = 0;
    for (std::size_t s : sentElems) {
        most = s > most ? s : most;
    }
    return most;   // elements sent by the busiest rank
}

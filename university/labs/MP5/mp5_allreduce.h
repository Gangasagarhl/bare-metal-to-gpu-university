// MP5 starter library: a chunked ring all-reduce written against a small "link" interface,
// so the same algorithm runs over threads (this lab), MPI (mp5_ring_mpi.cc) and, later,
// GPU peer copies or verbs (milestones 1-2 on real hardware).
// It is F8-05's Listing 1 reorganised for the project:
//   - Link<T>::sendRecv(send to next, receive from previous) is the only transport call;
//   - every wait has a deadline, and a timeout aborts the whole ring (milestone 4, F8);
//   - Mutant M injects one known bug, so the test suite can prove it catches it.
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <span>
#include <type_traits>
#include <vector>

namespace mp5 {

enum class Status { ok, timeout, aborted, stopped };

inline const char* name(Status s)
{
    switch (s) {
    case Status::ok:      return "ok";
    case Status::timeout: return "timeout";
    case Status::aborted: return "aborted";
    case Status::stopped: return "stopped (injected)";
    }
    return "?";
}

// Known bugs, one per value. Mutant::none is the real algorithm.
enum class Mutant {
    none,
    gather_short,     // all-gather runs one step too few
    chunks_floor,     // number of chunks rounded down instead of up
    split_floor,      // segment length count / n: the remainder is never reduced
    recv_segment,     // reduce-scatter adds the received chunk into the wrong segment
};

// The only transport operation: send ns elements to the next rank and receive nr
// elements from the previous rank. Counts elements sent, for the traffic check.
template <typename T>
class Link
{
public:
    virtual ~Link() = default;
    virtual Status sendRecv(const T* send, std::size_t ns, T* recv, std::size_t nr) = 0;
    std::size_t sentElems = 0;
};

struct Range
{
    std::size_t begin;
    std::size_t end;
};

// Segment g of n: elements [g*count/n, (g+1)*count/n). Lengths differ by at most one.
inline Range segment(std::size_t count, std::size_t n, std::size_t g, Mutant m)
{
    if (m == Mutant::split_floor) {
        const std::size_t len = count / n;           // BUG: drops count % n elements
        return {g * len, (g + 1) * len};
    }
    return {g * count / n, (g + 1) * count / n};
}

// Chunk k of `chunks` inside a segment.
inline Range chunk(Range seg, std::size_t k, std::size_t chunks)
{
    const std::size_t len = seg.end - seg.begin;
    return {seg.begin + k * len / chunks, seg.begin + (k + 1) * len / chunks};
}

// Ring all-reduce (sum) of `data` on rank r of n. Phase 0: reduce-scatter, n-1 steps;
// phase 1: all-gather, n-1 steps. stopAt >= 0 makes this rank stop silently before global
// step stopAt (a crashed or hung process, for the fault tests).
template <typename T, Mutant M = Mutant::none>
Status ringAllReduce(Link<T>& link, std::size_t r, std::size_t n, std::span<T> data,
                     std::size_t chunkElems, int stopAt = -1)
{
    if (n == 1) {
        return Status::ok;
    }
    const std::size_t count = data.size();
    const std::size_t maxSeg = (count + n - 1) / n;
    std::size_t chunks = (maxSeg + chunkElems - 1) / chunkElems;
    if (M == Mutant::chunks_floor) {
        chunks = maxSeg / chunkElems;                 // BUG: 0 when a segment is smaller than a chunk
    }
    if (chunks == 0 && M != Mutant::chunks_floor) {
        chunks = 1;                                   // empty segments still take part in every step
    }
    std::vector<T> got;
    int globalStep = 0;
    for (int phase = 0; phase < 2; ++phase) {
        std::size_t steps = n - 1;
        if (M == Mutant::gather_short && phase == 1) {
            steps = n - 2;                            // BUG
        }
        for (std::size_t step = 0; step < steps; ++step, ++globalStep) {
            if (globalStep == stopAt) {
                return Status::stopped;
            }
            const std::size_t sendSeg = (r + 2 * n - step + phase) % n;
            std::size_t recvSeg = (r + 2 * n - step - 1 + phase) % n;
            if (M == Mutant::recv_segment && phase == 0) {
                recvSeg = sendSeg;                    // BUG
            }
            for (std::size_t k = 0; k < chunks; ++k) {
                const Range s = chunk(segment(count, n, sendSeg, M), k, chunks);
                const Range d = chunk(segment(count, n, recvSeg, M), k, chunks);
                got.resize(d.end - d.begin);
                const Status st = link.sendRecv(data.data() + s.begin, s.end - s.begin,
                                                got.data(), got.size());
                if (st != Status::ok) {
                    return st;
                }
                for (std::size_t i = 0; i < got.size(); ++i) {
                    if (phase == 0) {
                        data[d.begin + i] += got[i];  // reduce
                    } else {
                        data[d.begin + i] = got[i];   // gather
                    }
                }
            }
        }
    }
    return Status::ok;
}

// ---------------------------------------------------------------------------------------
// In-process transport: n threads play n GPUs; rank r owns mailbox r, filled by rank r-1.
// Every wait ends at a deadline; the first rank that times out aborts the whole ring,
// which wakes every other rank (the stand-in for a communicator abort, F8-16).
template <typename T>
class ThreadRing
{
public:
    ThreadRing(std::size_t n, std::chrono::milliseconds timeout) : boxes_(n), timeout_(timeout) {}

    class ThreadLink : public Link<T>
    {
    public:
        ThreadLink(ThreadRing& ring, std::size_t r) : ring_(ring), r_(r) {}
        Status sendRecv(const T* send, std::size_t ns, T* recv, std::size_t nr) override
        {
            const std::size_t n = ring_.boxes_.size();
            Status st = ring_.put((r_ + 1) % n, std::vector<T>(send, send + ns));
            if (st != Status::ok) {
                return st;
            }
            this->sentElems += ns;
            std::vector<T> in;
            st = ring_.take(r_, in);
            if (st != Status::ok) {
                return st;
            }
            if (in.size() != nr) {                // the two ends disagree about the layout
                ring_.abort();
                return Status::aborted;
            }
            for (std::size_t i = 0; i < nr; ++i) {
                recv[i] = in[i];
            }
            return Status::ok;
        }

    private:
        ThreadRing& ring_;
        std::size_t r_;
    };

    void abort()
    {
        aborted_.store(true);
        for (Box& b : boxes_) {
            { std::lock_guard<std::mutex> lock(b.m); }   // no waiter can miss the flag
            b.cv.notify_all();
        }
    }
    bool aborted() const { return aborted_.load(); }

private:
    struct Box
    {
        std::mutex m;
        std::condition_variable cv;
        std::deque<std::vector<T>> q;
    };
    static constexpr std::size_t kCapacity = 2;       // flow control: at most two chunks in flight

    Status put(std::size_t to, std::vector<T> chunkData)
    {
        Box& b = boxes_[to];
        std::unique_lock<std::mutex> lock(b.m);
        const auto deadline = std::chrono::steady_clock::now() + timeout_;
        if (!b.cv.wait_until(lock, deadline, [&] { return aborted_.load() || b.q.size() < kCapacity; })) {
            lock.unlock();
            abort();
            return Status::timeout;
        }
        if (aborted_.load()) {
            return Status::aborted;
        }
        b.q.push_back(std::move(chunkData));
        b.cv.notify_all();
        return Status::ok;
    }
    Status take(std::size_t me, std::vector<T>& out)
    {
        Box& b = boxes_[me];
        std::unique_lock<std::mutex> lock(b.m);
        const auto deadline = std::chrono::steady_clock::now() + timeout_;
        if (!b.cv.wait_until(lock, deadline, [&] { return aborted_.load() || !b.q.empty(); })) {
            lock.unlock();
            abort();
            return Status::timeout;
        }
        if (aborted_.load()) {
            return Status::aborted;
        }
        out = std::move(b.q.front());
        b.q.pop_front();
        b.cv.notify_all();
        return Status::ok;
    }

    std::vector<Box> boxes_;
    std::chrono::milliseconds timeout_;
    std::atomic<bool> aborted_{false};
};

// Deterministic test data: integer values whose every partial sum is exact, and floats in
// [-1, 1) from a fixed hash of (rank, index).
inline std::uint64_t mix(std::uint64_t z)
{
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

template <typename T>
T inputValue(std::size_t rank, std::size_t i)
{
    if constexpr (std::is_integral_v<T>) {
        return static_cast<T>((static_cast<long>(rank) + 1) * (static_cast<long>(i % 251) - 125));
    } else {
        return static_cast<T>(static_cast<double>(mix(rank * 0x100000001ull + i) >> 11) / 4503599627370496.0 - 1.0);
    }
}

}  // namespace mp5

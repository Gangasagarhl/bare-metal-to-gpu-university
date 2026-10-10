// F8-05 Listing 1: a chunked, pipelined ring all-reduce with one thread per rank.
// Each rank owns one buffer. Rank r sends only to rank r+1 and receives only from rank r-1,
// through a small mailbox (the stand-in for a GPU link). Phase 1, reduce-scatter: N-1 steps,
// each adding one segment from the left neighbour. Phase 2, all-gather: N-1 steps, each
// copying one finished segment from the left neighbour. Every segment is cut into the same
// number of chunks, so the receiver can add chunk k while the sender already sends chunk k+1.
// Run without arguments: the acceptance tests. With "big": 1 GiB tests and timings (run.sh).
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
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

static bool g_timing = false;   // print times only in the -O2 "big" run

struct Range
{
    std::size_t begin;
    std::size_t end;
};

// chunk k of segment g: the same number of chunks in every segment (some may be empty)
Range chunkRange(std::size_t count, std::size_t n, std::size_t g, std::size_t k, std::size_t chunks)
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

// every rank's buffer must equal the exact sum; inputs keep every partial sum exact
template <typename T>
bool testOnce(std::size_t n, std::size_t bytes, std::size_t chunkElems, const char* type, bool print)
{
    const std::size_t count = bytes / sizeof(T);
    std::vector<std::vector<T>> buf(n, std::vector<T>(count));
    for (std::size_t r = 0; r < n; ++r) {
        for (std::size_t i = 0; i < count; ++i) {
            buf[r][i] = static_cast<T>((r + 1) * (i % 251));
        }
    }
    const auto t0 = std::chrono::steady_clock::now();
    const std::size_t sent = ringAllReduce(buf, chunkElems);
    const double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const T ranksSum = static_cast<T>(n * (n + 1) / 2);
    bool ok = true;
    for (std::size_t r = 0; r < n && ok; ++r) {
        for (std::size_t i = 0; i < count; ++i) {
            if (buf[r][i] != ranksSum * static_cast<T>(i % 251)) {
                ok = false;
                break;
            }
        }
    }
    if (print) {
        const double expectSent = 2.0 * static_cast<double>(n - 1) / static_cast<double>(n) * static_cast<double>(count);
        std::printf("%-5s N=%zu %11zu bytes: %s; busiest rank sent %zu elements (2(N-1)/N x count = %.1f)",
                    type, n, bytes, ok ? "PASS" : "FAIL", sent, expectSent);
        if (g_timing) {
            const double busbw = static_cast<double>(bytes) / sec / 1e9 * 2.0 * static_cast<double>(n - 1) / static_cast<double>(n);
            std::printf("; %.3f s, busbw %.2f GB/s", sec, busbw);
        }
        std::printf("\n");
    }
    return ok;
}

// random floats: the ring's sum is not the sequential sum, but all ranks agree bit for bit
void roundingDemo()
{
    const std::size_t n = 4, count = 100000;
    std::vector<std::vector<float>> buf(n, std::vector<float>(count));
    std::uint32_t x = 12345;
    for (auto& b : buf) {
        for (float& v : b) {
            x = x * 1664525u + 1013904223u;                 // a fixed pseudo-random sequence
            v = static_cast<float>(x >> 8) / 16777216.0f;    // in [0, 1)
        }
    }
    std::vector<float> seq(count, 0.0f);
    for (const auto& b : buf) {
        for (std::size_t i = 0; i < count; ++i) {
            seq[i] += b[i];                                  // rank 0 + rank 1 + rank 2 + rank 3
        }
    }
    ringAllReduce(buf, 1000);
    std::size_t differ = 0;
    float maxDiff = 0.0f;
    bool sameOnAll = true;
    for (std::size_t i = 0; i < count; ++i) {
        const float d = buf[0][i] > seq[i] ? buf[0][i] - seq[i] : seq[i] - buf[0][i];
        differ += (d != 0.0f);
        maxDiff = d > maxDiff ? d : maxDiff;
        for (std::size_t r = 1; r < n; ++r) {
            sameOnAll = sameOnAll && (std::memcmp(&buf[r][i], &buf[0][i], sizeof(float)) == 0);
        }
    }
    std::printf("random floats, N=4, %zu elements: %zu differ from the sequential sum (largest difference %g); "
                "all ranks bit-identical: %s\n", count, differ, static_cast<double>(maxDiff), sameOnAll ? "yes" : "no");
}

int main(int argc, char** argv)
{
    const bool big = argc > 1 && std::strcmp(argv[1], "big") == 0;
    bool ok = true;
    g_timing = big;
    if (!big) {
        const std::size_t chunk = 16384;                     // elements per chunk (64 KiB of 4-byte data)
        for (std::size_t n : {2u, 4u, 8u}) {
            for (std::size_t bytes : {std::size_t{4}, std::size_t{12}, std::size_t{1028}, std::size_t{1} << 20,
                                      std::size_t{16} << 20}) {
                ok = testOnce<std::int32_t>(n, bytes, chunk, "int32", true) && ok;
                ok = testOnce<float>(n, bytes, chunk, "float", true) && ok;
            }
        }
        roundingDemo();
    } else {
        const std::size_t chunk = 262144;                    // 1 MiB chunks of 4-byte data
        for (std::size_t n : {2u, 4u}) {
            ok = testOnce<std::int32_t>(n, std::size_t{1} << 30, chunk, "int32", true) && ok;
            ok = testOnce<float>(n, std::size_t{1} << 30, chunk, "float", true) && ok;
        }
        for (std::size_t bytes = std::size_t{1} << 20; bytes <= (std::size_t{256} << 20); bytes *= 4) {
            ok = testOnce<float>(4, bytes, chunk, "float", true) && ok;
        }
    }
    std::printf("acceptance: %s\n", ok ? "all tests PASS" : "some tests FAILED");
    return ok ? 0 : 1;
}

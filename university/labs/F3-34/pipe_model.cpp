// pipe_model.cpp - F3-34: the pipe object of a kernel, modelled in host C++. The mutex stands for
// the kernel lock of the pipe and each condition variable for one wait queue.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

constexpr long E_AGAIN = -11, E_PIPE = -32;          // returned as negative numbers, after POSIX names

class Pipe
{
public:
    explicit Pipe(std::size_t capacity) : buf_(capacity) {}

    long read(void* out, std::size_t n, bool nonblock)
    {
        std::unique_lock<std::mutex> lk(m_);
        while (count_ == 0) {
            if (writers_ == 0) return 0;               // no data and no writer: end of file
            if (nonblock) return E_AGAIN;
            ++sleeping_readers_;
            readable_.wait(lk);                        // releases the lock while asleep
            --sleeping_readers_;
        }
        std::size_t k = n < count_ ? n : count_;       // a read returns what is there, up to n
        for (std::size_t i = 0; i < k; ++i) static_cast<std::uint8_t*>(out)[i] = buf_[(head_ + i) % buf_.size()];
        head_ = (head_ + k) % buf_.size();
        count_ -= k;
        writable_.notify_all();                        // space appeared: wake every sleeping writer
        return static_cast<long>(k);
    }

    long write(const void* in, std::size_t n, bool nonblock)
    {
        std::unique_lock<std::mutex> lk(m_);
        std::size_t done = 0;
        while (done < n) {                             // a blocking write finishes the whole request
            if (readers_ == 0) return done ? static_cast<long>(done) : E_PIPE;
            if (count_ == buf_.size()) {
                if (nonblock) return done ? static_cast<long>(done) : E_AGAIN;
                ++sleeping_writers_;
                writable_.wait(lk);
                --sleeping_writers_;
                continue;
            }
            std::size_t k = std::min(n - done, buf_.size() - count_);
            for (std::size_t i = 0; i < k; ++i)
                buf_[(head_ + count_ + i) % buf_.size()] = static_cast<const std::uint8_t*>(in)[done + i];
            count_ += k;
            done += k;
            readable_.notify_all();                    // data appeared: wake every sleeping reader
        }
        return static_cast<long>(done);
    }

    void close_write() { std::lock_guard<std::mutex> g(m_); --writers_; readable_.notify_all(); }
    void close_read() { std::lock_guard<std::mutex> g(m_); --readers_; writable_.notify_all(); }
    void add_reader() { std::lock_guard<std::mutex> g(m_); ++readers_; }

    void report()                                      // for the watchdog: the pipe's state
    {
        std::lock_guard<std::mutex> g(m_);
        std::printf("  pipe: %zu of %zu bytes used, readers %d (asleep %d), writers %d (asleep %d)\n",
                    count_, buf_.size(), readers_, sleeping_readers_, writers_, sleeping_writers_);
    }

private:
    std::mutex m_;
    std::condition_variable readable_, writable_;
    std::vector<std::uint8_t> buf_;
    std::size_t head_ = 0, count_ = 0;
    int readers_ = 1, writers_ = 1, sleeping_readers_ = 0, sleeping_writers_ = 0;
};

static int failures = 0;
static void expect(bool ok, const char* what)
{
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) ++failures;
}

static std::uint64_t next_word(std::uint64_t& s)      // xorshift64: a reproducible byte stream
{
    s ^= s << 13; s ^= s >> 7; s ^= s << 17;
    return s;
}

// Watchdog: if `progress` stops changing for 2 s, print the pipes' state and stop the program.
static void watchdog(std::atomic<std::uint64_t>& progress, std::atomic<bool>& done, std::vector<Pipe*> pipes)
{
    std::uint64_t last = progress.load();
    int still = 0;
    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::uint64_t now = progress.load();
        still = now == last ? still + 1 : 0;
        last = now;
        if (still == 20) {
            std::printf("WATCHDOG: no progress for 2 s (%llu bytes moved so far)\n", static_cast<unsigned long long>(now));
            for (Pipe* p : pipes) p->report();
            std::fflush(stdout);
            std::_Exit(3);
        }
    }
}

int main()
{
    std::printf("== semantics ==\n");
    {
        Pipe p(16);
        char b[32];
        expect(p.read(b, 4, true) == E_AGAIN, "non-blocking read of an empty pipe gives -EAGAIN");
        expect(p.write("hello world", 11, false) == 11, "write of 11 bytes into a 16-byte pipe");
        expect(p.read(b, 32, false) == 11, "a read returns what is there (11), not the 32 asked for");
        expect(p.write("0123456789abcdefXYZ", 19, true) == 16, "non-blocking write into a full pipe stops at 16");
        expect(p.write("Z", 1, true) == E_AGAIN, "then -EAGAIN while it stays full");
        p.read(b, 32, false);
        p.close_write();
        expect(p.read(b, 4, false) == 0, "no data and no writers: read returns 0 (end of file)");
        Pipe q(16);
        q.close_read();
        expect(q.write("x", 1, false) == E_PIPE, "write with no readers left gives -EPIPE");
    }

    std::printf("== pipeline of three threads, 32 MiB through two 4 KiB pipes ==\n");
    const std::uint64_t total = 32ull << 20;
    Pipe p1(4096), p2(4096);
    std::atomic<std::uint64_t> progress{0};
    std::atomic<bool> done{false};
    std::thread dog(watchdog, std::ref(progress), std::ref(done), std::vector<Pipe*>{&p1, &p2});
    std::uint64_t sent_sum = 0, got_sum = 0, got = 0;
    std::thread gen([&] {
        std::uint64_t s = 304, buf[512];
        for (std::uint64_t off = 0; off < total; off += sizeof buf) {
            for (auto& w : buf) { w = next_word(s); sent_sum += w; }
            p1.write(buf, sizeof buf, false);
        }
        p1.close_write();
    });
    std::thread filter([&] {                           // passes data through, like "cat"
        std::uint8_t buf[1000];
        long n;
        while ((n = p1.read(buf, sizeof buf, false)) > 0) p2.write(buf, static_cast<std::size_t>(n), false);
        p2.close_write();
    });
    std::thread sink([&] {
        std::vector<std::uint8_t> all;
        std::uint8_t buf[3000], word[8];
        std::size_t have = 0;
        long n;
        while ((n = p2.read(buf, sizeof buf, false)) > 0) {
            for (long i = 0; i < n; ++i) {             // re-assemble 8-byte words across reads
                word[have++] = buf[i];
                if (have == 8) { std::uint64_t w; std::memcpy(&w, word, 8); got_sum += w; have = 0; }
            }
            got += static_cast<std::uint64_t>(n);
            progress = got;
        }
    });
    gen.join(); filter.join(); sink.join();
    done = true;
    dog.join();
    std::printf("sent %llu bytes, received %llu bytes\n", static_cast<unsigned long long>(total),
                static_cast<unsigned long long>(got));
    expect(got == total && got_sum == sent_sum, "every byte arrived (sum of 64-bit words matches)");

    std::printf("== one writer, two workers taking 100000 records of 64 bytes each ==\n");
    Pipe jobs(4096);
    jobs.add_reader();                                 // two readers share the pipe
    std::atomic<std::uint64_t> taken{0};
    std::atomic<bool> done2{false};
    std::thread dog2(watchdog, std::ref(taken), std::ref(done2), std::vector<Pipe*>{&jobs});
    const int per_worker = 100000;
    std::thread writer([&] {
        std::uint8_t rec[64] = {};
        for (int i = 0; i < 2 * per_worker; ++i) jobs.write(rec, sizeof rec, false);
        jobs.close_write();
    });
    auto worker = [&] {
        std::uint8_t rec[64];
        for (int i = 0; i < per_worker; ++i) {         // exactly one record per job
            std::size_t have = 0;
            while (have < sizeof rec) have += static_cast<std::size_t>(jobs.read(rec + have, sizeof rec - have, false));
            taken += sizeof rec;
        }
    };
    std::thread w1(worker), w2(worker);
    writer.join(); w1.join(); w2.join();
    done2 = true;
    dog2.join();
    expect(taken == 2ull * per_worker * 64, "both workers got all their records");
    std::printf("pipe model: %d failures\n", failures);
    return failures == 0 ? 0 : 1;
}

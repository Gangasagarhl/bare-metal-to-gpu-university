// prodcons.cpp - the B10 bounded buffer on the host, with std::mutex and two
// std::condition_variable objects, checked for lost and duplicated items.
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

class BoundedBuffer {
public:
    explicit BoundedBuffer(std::size_t capacity) : slots_(capacity) {}

    void put(long v)
    {
        std::unique_lock<std::mutex> lk(m_);
        not_full_.wait(lk, [this] { return count_ < slots_.size(); });   // loop guards spurious wakeups
        slots_[(head_ + count_) % slots_.size()] = v;
        ++count_;
        not_empty_.notify_one();
    }

    long get()
    {
        std::unique_lock<std::mutex> lk(m_);
        not_empty_.wait(lk, [this] { return count_ > 0; });
        long v = slots_[head_];
        head_ = (head_ + 1) % slots_.size();
        --count_;
        not_full_.notify_one();
        return v;
    }

private:
    std::mutex m_;
    std::condition_variable not_full_, not_empty_;
    std::vector<long> slots_;
    std::size_t head_ = 0, count_ = 0;
};

int main()
{
    constexpr long kItems = 10'000'000;
    constexpr int kProducers = 4, kConsumers = 4;
    BoundedBuffer buf(64);
    std::vector<unsigned char> seen(kItems, 0);
    std::vector<long> dup(kConsumers, 0);
    std::vector<std::thread> ts;
    for (int p = 0; p < kProducers; ++p) {
        ts.emplace_back([&, p] {
            for (long i = p; i < kItems; i += kProducers) buf.put(i);
        });
    }
    for (int c = 0; c < kConsumers; ++c) {
        ts.emplace_back([&, c] {
            for (;;) {
                long v = buf.get();
                if (v < 0) return;                         // stop marker
                if (seen[static_cast<std::size_t>(v)]++) ++dup[static_cast<std::size_t>(c)];
            }
        });
    }
    for (int p = 0; p < kProducers; ++p) ts[static_cast<std::size_t>(p)].join();
    for (int c = 0; c < kConsumers; ++c) buf.put(-1);      // one stop marker per consumer
    for (int c = 0; c < kConsumers; ++c) ts[static_cast<std::size_t>(kProducers + c)].join();
    long missing = 0, dups = 0;
    for (unsigned char s : seen) missing += (s == 0);
    for (long d : dup) dups += d;
    std::printf("%ld items, %d producers, %d consumers, buffer of 64\n", kItems, kProducers, kConsumers);
    std::printf("missing %ld, duplicated %ld -> %s\n", missing, dups, (missing == 0 && dups == 0) ? "PASS" : "FAIL");
    return (missing == 0 && dups == 0) ? 0 : 1;
}

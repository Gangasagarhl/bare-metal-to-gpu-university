// uorb_threads.cpp - the same topic used by two real threads at once.
// The publisher writes 200,000 messages as fast as it can; the subscriber reads
// whenever it gets the processor. Which messages it misses changes from run to run,
// so the program prints only things that must always hold.
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <thread>

#include "uorb_lite.h"

namespace {

struct Msg
{
    std::uint64_t seq;
    std::uint64_t a;
    std::uint64_t b;
    std::uint64_t check;   // seq ^ a ^ b: a torn (half-written) copy would fail this
};

constexpr std::uint64_t kCount = 200000;

} // namespace

int main()
{
    uorb_lite::Topic<Msg, 4> topic("stress_model");
    uorb_lite::Subscription<Msg, 4> sub(topic);
    std::atomic<bool> done{false};
    std::uint64_t received = 0;
    std::uint64_t torn = 0;
    std::uint64_t outOfOrder = 0;
    std::uint64_t lastSeen = 0;

    std::thread reader([&] {
        std::uint64_t last = 0;
        bool first = true;
        Msg m{};
        for (;;) {
            const bool finished = done.load();
            while (sub.update(m)) {
                ++received;
                if ((m.seq ^ m.a ^ m.b) != m.check) {
                    ++torn;
                }
                if (!first && m.seq <= last) {
                    ++outOfOrder;
                }
                first = false;
                last = m.seq;
            }
            if (finished) {
                lastSeen = last;
                break;
            }
            std::this_thread::yield();
        }
    });

    for (std::uint64_t i = 1; i <= kCount; ++i) {
        const std::uint64_t a = i * 2654435761u;
        const std::uint64_t b = ~i;
        topic.publish(Msg{i, a, b, i ^ a ^ b});
    }
    done.store(true);
    reader.join();

    const bool sumOk = received + sub.lost() == kCount;
    std::printf("published %llu messages\n", static_cast<unsigned long long>(kCount));
    std::printf("received + lost == published: %s\n", sumOk ? "yes" : "NO");
    std::printf("torn messages: %llu\n", static_cast<unsigned long long>(torn));
    std::printf("out-of-order messages: %llu\n", static_cast<unsigned long long>(outOfOrder));
    std::printf("the newest message was received: %s\n", lastSeen == kCount ? "yes" : "NO");
    return (sumOk && torn == 0 && outOfOrder == 0 && lastSeen == kCount) ? 0 : 1;
}

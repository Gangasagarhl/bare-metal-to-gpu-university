// rx_ring.cpp - SE301 F12-01: the receive ring written FROM the plan in plan.txt.
// Every test is named after the plan line it checks (P1..P5), so a failing test
// points back to a decision that was written down before the code.
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>

class RxRing
{
public:
    static constexpr std::size_t kSize = 8;                    // D1: a power of two

    // start: where both counters begin; only the test for P5 passes a value
    explicit RxRing(std::size_t start = 0) : head_(start), tail_(start) {}

    bool push(std::uint8_t b)                                   // producer (interrupt)
    {
        if (head_ - tail_ == kSize) {                           // D2: full
            ++dropped_;                                         // F2: drop the new byte, count it
            return false;
        }
        buf_[head_ % kSize] = b;                                // D3
        ++head_;
        return true;
    }

    bool pop(std::uint8_t& b)                                   // consumer (console task)
    {
        if (head_ == tail_) {                                   // D2: empty; F1: never block
            return false;
        }
        b = buf_[tail_ % kSize];
        ++tail_;
        return true;
    }

    std::size_t size() const { return head_ - tail_; }         // D2 (wraps correctly: F3)
    std::size_t dropped() const { return dropped_; }

private:
    std::array<std::uint8_t, kSize> buf_{};
    std::size_t head_;
    std::size_t tail_;
    std::size_t dropped_ = 0;
};

static std::string drain(RxRing& r)
{
    std::string s;
    std::uint8_t b = 0;
    while (r.pop(b)) {
        s += static_cast<char>(b);
    }
    return s;
}

static void fill(RxRing& r, const std::string& s)
{
    for (char c : s) {
        r.push(static_cast<std::uint8_t>(c));
    }
}

static int report(const char* id, const char* what, bool ok)
{
    std::printf("%s %-58s %s\n", id, what, ok ? "ok" : "FAILED");
    return ok ? 0 : 1;
}

int main()
{
    int failed = 0;
    {
        RxRing r;
        std::uint8_t b = 0;
        failed += report("P1", "pop() on a new ring returns false", !r.pop(b));
    }
    {
        RxRing r;
        fill(r, "abcde");
        const std::string first = drain(r);
        fill(r, "fghij");                                      // crosses the end of the array
        failed += report("P2", "order kept across the end of the array",
                         first == "abcde" && drain(r) == "fghij");
    }
    {
        RxRing r;
        fill(r, "make run!");                                  // 9 bytes into 8 slots
        const std::size_t held = r.size();
        const std::size_t lost = r.dropped();
        failed += report("P3", "9 pushes: 8 held, 1 dropped, the first 8 kept",
                         held == 8 && lost == 1 && drain(r) == "make run");
    }
    {
        RxRing r;
        fill(r, "12345678");
        failed += report("P4", "exactly 8 pushes: size() is 8, not 0", r.size() == 8);
    }
    {
        RxRing r(std::numeric_limits<std::size_t>::max() - 2);
        fill(r, "wrapping");
        failed += report("P5", "counters wrap past their largest value", drain(r) == "wrapping");
    }
    std::printf("%d of 5 plan checks failed\n", failed);
    return failed == 0 ? 0 : 1;
}

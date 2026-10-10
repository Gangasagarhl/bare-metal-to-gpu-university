// F1-42: a tick-by-tick model of a UART receiver and the byte stream feeding it.
// Exercise numbers, not a real part: one byte arrives every kGap ticks; the
// receiver holds at most `depth` bytes (depth 1 = a single holding register).
// A byte that arrives when the receiver is full is lost and the overrun flag
// is set, as the receive-overrun idea in UART datasheets describes.
#pragma once
#include <cstdint>
#include <deque>

constexpr int kGap = 10;                 // ticks between arriving bytes
constexpr int kTotal = 64 * 1024;        // 64 KiB, as in curriculum milestone C2

inline std::uint8_t pattern(int i)       // the bytes the sender transmits
{
    return static_cast<std::uint8_t>((i * 7 + 3) & 0xFF);
}

struct Uart
{
    std::deque<std::uint8_t> rx;         // receive holding register / FIFO
    std::size_t depth = 1;
    bool overrun = false;                // like an "overrun error" status bit
    long lost = 0;

    // The line delivers byte b. Returns false if it had to be dropped.
    bool deliver(std::uint8_t b)
    {
        if (rx.size() >= depth) {
            overrun = true;
            ++lost;
            return false;
        }
        rx.push_back(b);
        return true;
    }
    bool data_ready() const { return !rx.empty(); }
    std::uint8_t read()                  // reading the data register empties one slot
    {
        const std::uint8_t b = rx.front();
        rx.pop_front();
        return b;
    }
};

struct Checksum                          // order-sensitive, so a lost byte shows
{
    std::uint32_t a = 1, b = 0;
    void add(std::uint8_t x)
    {
        a = (a + x) % 65521u;
        b = (b + a) % 65521u;
    }
    std::uint32_t value() const { return (b << 16) | a; }
};

// The "field" workload of the forensic lab: the main loop normally needs 4
// ticks per pass, but every 8th pass it also flushes a log, which takes 40.
inline int field_work(long pass)
{
    return (pass % 8 == 7) ? 40 : 4;
}

inline std::uint32_t expected_checksum()
{
    Checksum c;
    for (int i = 0; i < kTotal; ++i) {
        c.add(pattern(i));
    }
    return c.value();
}

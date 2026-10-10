// HW204 final exam, forensic evidence "The FIFO that was not enough" (Lab Engineer;
// the output is shown to candidates, this source is in the key).
// The course's UART model (uart_model.h, from F1-42) on a new board: the UART now has a
// 4-byte receive FIFO instead of a 1-byte holding register, the firmware still polls once
// per pass of its main loop, and a pass normally takes 4 ticks, but every 16th pass also
// refreshes a display and flushes a log, which takes 60 ticks. The bench pastes 64 KiB at
// one byte per 10 ticks (kGap) and records the same four BENCH reports as F1-42's bench tool.
// Exercise numbers (owner ruling A4), not a real part's.
#include <algorithm>
#include <cstdio>
#include <map>
#include <vector>

#include "uart_model.h"

constexpr std::size_t kFifoDepth = 4;

static int board2_work(long pass)       // ticks until the main loop polls the UART again
{
    return (pass % 16 == 15) ? 60 : 4;
}

int main()
{
    Uart u;
    u.depth = kFifoDepth;
    Checksum echo;
    long echoed = 0;
    int sent = 0;
    int next_poll = 0;
    long pass = 0;
    int last_poll = 0;
    std::map<int, long> pass_len;
    std::vector<int> overrun_at;
    std::vector<int> poll_at;
    std::size_t fifo_high = 0;
    for (int t = 0; sent < kTotal || u.data_ready(); ++t) {
        if (sent < kTotal && t % kGap == 0) {
            if (!u.deliver(pattern(sent))) overrun_at.push_back(t);
            ++sent;
            fifo_high = std::max(fifo_high, u.rx.size());
        }
        if (t >= next_poll) {
            if (t > 0) ++pass_len[t - last_poll];
            last_poll = t;
            poll_at.push_back(t);
            while (u.data_ready()) {
                echo.add(u.read());
                ++echoed;
            }
            next_poll = t + board2_work(pass);
            ++pass;
        }
    }
    std::printf("BENCH 1: paste test, %d bytes sent at one byte per %d ticks; UART receive FIFO %zu bytes\n",
                kTotal, kGap, kFifoDepth);
    std::printf("  received %ld bytes, checksum 0x%08X, expected 0x%08X -> %s\n", echoed,
                static_cast<unsigned>(echo.value()), static_cast<unsigned>(expected_checksum()),
                echo.value() == expected_checksum() ? "PASS" : "FAIL");
    std::printf("BENCH 2: UART overrun flag (OE) seen %zu times; first six at t =", overrun_at.size());
    for (std::size_t i = 0; i < 6 && i < overrun_at.size(); ++i) std::printf(" %d", overrun_at[i]);
    std::printf("; FIFO high-water mark %zu bytes\n", fifo_high);
    std::printf("BENCH 3: main loop pass length (ticks between two UART polls)\n");
    for (const auto& [len, count] : pass_len) std::printf("  %3d ticks: %7ld passes\n", len, count);
    std::printf("BENCH 4: polls around the first overrun:");
    const int first = overrun_at.empty() ? 0 : overrun_at.front();
    for (int p : poll_at) if (p > first - 80 && p < first + 40) std::printf(" %d", p);
    std::printf("\n");
    return 0;
}

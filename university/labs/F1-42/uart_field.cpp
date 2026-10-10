// F1-42 forensic evidence: the shipped firmware (polling) on the bench.
// A 64 KiB file is pasted into the serial port; the firmware echoes it. The
// firmware's main loop polls the UART once per pass; a pass normally takes
// 4 ticks, but every 8th pass also flushes the log (see field_work()).
// The bench tool records: the paste test result, every overrun the UART
// reported, and the length of the main loop's passes.
#include <algorithm>
#include <cstdio>
#include <map>
#include <vector>

#include "uart_model.h"

int main()
{
    Uart u;
    Checksum echo;
    long echoed = 0;
    int sent = 0;
    int next_poll = 0;
    long pass = 0;
    int last_poll = 0;
    std::map<int, long> pass_len;          // ticks between polls -> how often
    std::vector<int> overrun_at;
    std::vector<int> poll_at;
    for (int t = 0; sent < kTotal || u.data_ready(); ++t) {
        if (sent < kTotal && t % kGap == 0) {
            if (!u.deliver(pattern(sent))) {
                overrun_at.push_back(t);
            }
            ++sent;
        }
        if (t >= next_poll) {
            if (t > 0) {
                ++pass_len[t - last_poll];
            }
            last_poll = t;
            poll_at.push_back(t);
            while (u.data_ready()) {
                echo.add(u.read());
                ++echoed;
            }
            next_poll = t + field_work(pass);
            ++pass;
        }
    }
    std::printf("BENCH 1: paste test, %d bytes sent at one byte per %d ticks\n", kTotal, kGap);
    std::printf("  received %ld bytes, checksum 0x%08X, expected 0x%08X -> %s\n", echoed,
                static_cast<unsigned>(echo.value()), static_cast<unsigned>(expected_checksum()),
                echo.value() == expected_checksum() ? "PASS" : "FAIL");
    std::printf("BENCH 2: UART overrun flag (OE) seen %zu times; first five at t =", overrun_at.size());
    for (std::size_t i = 0; i < 5 && i < overrun_at.size(); ++i) {
        std::printf(" %d", overrun_at[i]);
    }
    std::printf("\nBENCH 3: main loop pass length (ticks between two UART polls)\n");
    for (const auto& [len, count] : pass_len) {
        std::printf("  %3d ticks: %7ld passes\n", len, count);
    }
    std::printf("BENCH 4: polls around the first overrun:");
    const int first = overrun_at.empty() ? 0 : overrun_at.front();
    for (int p : poll_at) {
        if (p > first - 60 && p < first + 40) {
            std::printf(" %d", p);
        }
    }
    std::printf("\n");
    return 0;
}

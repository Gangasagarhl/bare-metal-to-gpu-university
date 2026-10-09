// F1-42 Listing 1: UART echo by polling. The main loop does `work` ticks of
// other jobs, then polls the UART and echoes every byte it finds.
#include <cstdio>
#include <vector>

#include "uart_model.h"

struct Result
{
    long echoed = 0;
    long lost = 0;
    long empty_polls = 0;
    std::uint32_t sum = 0;
};

Result run(int work, bool show_events)
{
    Uart u;
    Checksum echo;
    Result r;
    int sent = 0;
    int next_poll = 0;
    int shown = 0;
    for (int t = 0; sent < kTotal || u.data_ready(); ++t) {
        if (sent < kTotal && t % kGap == 0) {           // the line delivers a byte
            if (!u.deliver(pattern(sent)) && show_events && shown < 4) {
                std::printf("  t=%6d  byte #%d arrived, receiver full -> OVERRUN, byte lost\n",
                            t, sent);
                ++shown;
            }
            ++sent;
        }
        if (t >= next_poll) {                            // main loop reaches the poll
            if (!u.data_ready()) {
                ++r.empty_polls;
            }
            while (u.data_ready()) {
                echo.add(u.read());
                ++r.echoed;
            }
            next_poll = t + work;                        // then busy with other jobs
        }
    }
    r.lost = u.lost;
    r.sum = echo.value();
    return r;
}

int main()
{
    const std::uint32_t want = expected_checksum();
    std::printf("sender: %d bytes, one every %d ticks; receiver holds 1 byte\n", kTotal, kGap);
    std::printf("expected checksum 0x%08X\n\n", static_cast<unsigned>(want));
    std::printf("%-12s %8s %8s %12s %12s  %s\n", "work/ticks", "echoed", "lost", "empty polls",
                "checksum", "result");
    for (int work : {4, 9, 10, 11, 25}) {
        const Result r = run(work, false);
        std::printf("%-12d %8ld %8ld %12ld   0x%08X  %s\n", work, r.echoed, r.lost,
                    r.empty_polls, static_cast<unsigned>(r.sum),
                    r.sum == want ? "PASS" : "FAIL (bytes missing)");
    }
    std::printf("\nfirst overrun events with work = 25:\n");
    run(25, true);
    return 0;
}

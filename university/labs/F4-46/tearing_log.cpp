// tearing_log.cpp - DR405 F4-46 forensic lab "Tearing": the evidence pack. Two frame-timing
// logs of the same animation, written by the flip model of flipsim.h: build A (healthy)
// and build B (the one users complain about). Render times vary with a fixed pseudo-random
// sequence so the run is repeatable. The log format is this course's own.
#include <cstdint>
#include "flipsim.h"

int main()
{
    const flip::Mode m = flip::kQemu640;
    std::vector<double> ready;
    uint32_t x = 12345;
    double t = 1000.0;
    for (int i = 0; i < 10; ++i) {
        x = x * 1103515245u + 12345u;                  // a fixed linear congruential sequence
        t += 9000.0 + static_cast<double>((x >> 8) % 6000);   // 9.0 to 15.0 ms per frame
        ready.push_back(t);
    }
    const char* names[2] = {"A", "B"};
    for (int b = 0; b < 2; ++b) {
        std::vector<flip::Refresh> rows;
        std::vector<flip::Flip> flips;
        flip::run(m, ready, b == 0, 13, rows, flips);
        std::printf("== frame-timing log, build %s (mode 640x480, frame period %.1f us) ==\n", names[b],
                    flip::frame_us(m));
        std::printf("seq  t_request_us  scanout_line_at_request  t_flip_done_us  scanout_line_at_done\n");
        for (const flip::Flip& f : flips) {
            if (f.dropped)
                std::printf("%3d  %12.1f  %23d  %14s  %20s\n", f.buffer, f.request_us, f.line_at_request, "(skipped)", "-");
            else
                std::printf("%3d  %12.1f  %23d  %14.1f  %20d\n", f.buffer, f.request_us, f.line_at_request,
                            f.latched_us, f.line_at_latch);
        }
        std::printf("\n");
    }
    return 0;
}

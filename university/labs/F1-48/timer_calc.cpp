// F1-48 Listing 1: choosing a prescaler and a reload value for a periodic
// timer interrupt. Model: an up-counter clocked at f_in / prescaler counts
// 0, 1, ..., reload and then wraps to 0 and raises the interrupt, so one period
// is (reload + 1) counts. The input clock is an exercise number, not a real part.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <initializer_list>

int main()
{
    const double f_in = 16'000'000.0;          // exercise: a 16 MHz timer input clock
    const double want_hz = 1000.0;             // we want a 1 kHz tick (1 ms)
    std::printf("input clock %.0f Hz, wanted tick %.0f Hz\n", f_in, want_hz);
    std::printf("prescaler  counts/period  reload  fits 16 bits?  actual Hz   error\n");
    for (int prescaler : {1, 8, 64, 256}) {
        const double counts = f_in / prescaler / want_hz;
        const long reload = std::lround(counts) - 1;
        const bool fits = reload >= 0 && reload <= 0xFFFF;
        const double actual = f_in / prescaler / static_cast<double>(reload + 1);
        std::printf("%9d  %13.2f  %6ld  %-13s  %9.3f  %+.4f %%\n", prescaler, counts, reload,
                    fits ? "yes" : "NO", actual, (actual - want_hz) / want_hz * 100.0);
    }
    // The classic off-by-one: writing the count (16000) instead of count - 1.
    const double wrong = f_in / 1 / 16001.0;
    std::printf("\nreload written as 16000 instead of 15999: %.4f Hz, error %+.4f %%\n", wrong,
                (wrong - want_hz) / want_hz * 100.0);
    const double drift_s = 86400.0 * (want_hz - wrong) / want_hz;
    std::printf("a clock counting these ticks loses %.2f s per day\n", drift_s);
    return 0;
}

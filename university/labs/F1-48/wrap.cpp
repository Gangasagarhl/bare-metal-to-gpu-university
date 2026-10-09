// F1-48 forensic evidence: a timeout check in firmware that keeps a 32-bit
// millisecond tick counter. The counter starts close to its maximum so the
// wrap-around happens during the test (it starts at 0 at power-on on a real
// board, so the bug appears only after the board has run for a long time).
#include <cstdint>
#include <cstdio>

bool expired_naive(std::uint32_t now, std::uint32_t deadline)
{
    return now >= deadline;
}

bool expired_wrap_safe(std::uint32_t now, std::uint32_t deadline)
{
    // The difference is computed modulo 2^32 and read as signed: correct as long
    // as the two times are less than 2^31 ticks apart.
    return static_cast<std::int32_t>(now - deadline) >= 0;
}

int main()
{
    const std::uint32_t start = 0xFFFFFFFFu - 30;   // 31 ms before the counter wraps
    const std::uint32_t deadline = start + 50;      // "wait 50 ms" (wraps to a small number)
    std::printf("start    = %10u (0x%08X)\ndeadline = %10u (0x%08X)  [start + 50, modulo 2^32]\n",
                start, start, deadline, deadline);
    std::printf("   ms  now(ticks)   naive  wrap-safe\n");
    for (std::uint32_t ms = 0; ms <= 60; ms += 10) {
        const std::uint32_t now = start + ms;
        std::printf("%5u  %10u  %6s  %9s\n", ms, now, expired_naive(now, deadline) ? "yes" : "no",
                    expired_wrap_safe(now, deadline) ? "yes" : "no");
    }
    const double days = 4294967296.0 / 1000.0 / 86400.0;
    std::printf("a 32-bit millisecond counter wraps every 2^32 ms = %.2f days\n", days);
    return 0;
}

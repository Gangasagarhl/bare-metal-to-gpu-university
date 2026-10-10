// fmt_host.cpp - F3-18: host unit test of the kernel's formatter (kformat.h).
// The kernel cannot use the C library, but the host can: every case is compared with
// std::snprintf, which serves as the reference implementation.
#include <cstdio>
#include <cstring>
#include <string>
#include "kformat.h"

int g_failures = 0;

template <typename... Args>
void check(const char* fmt, Args... args)
{
    char mine[128];
    char ref[128];
    ksnprintf(mine, sizeof mine, fmt, args...);
    std::snprintf(ref, sizeof ref, fmt, args...);
    bool ok = std::strcmp(mine, ref) == 0;
    std::printf("%-6s %-12s -> \"%s\"\n", ok ? "ok" : "FAIL", fmt, mine);
    if (!ok) {
        std::printf("       expected \"%s\"\n", ref);
        ++g_failures;
    }
}

int main()
{
    check("%d", 0);
    check("%d", -2147483647 - 1);
    check("%u", 4294967295u);
    check("%x", 0xdeadbeefu);
    check("%08x", 0x2badb002u);
    check("%lx", 0xffffffff80000000ul);
    check("%5d|", -42);
    check("%05d|", -42);
    check("%ld", -9223372036854775807l - 1);
    check("%lu", 18446744073709551615ul);
    check("[%s] [%6s]", "kernel", "ok");
    check("%c%c%c", 'B', '1', '!');
    check("[%.4s] [%.6s]", "APICxyz", "BOCHS ");
    check("[%6.2s]", "abcdef");
    check("100%% %s", "done");
    // truncation: the buffer is always terminated and never overrun
    char tiny[6];
    int n = ksnprintf(tiny, sizeof tiny, "%s", "overflowing");
    bool trunc_ok = n == 5 && std::strcmp(tiny, "overf") == 0;
    std::printf("%-6s truncation -> \"%s\" (%d chars kept)\n", trunc_ok ? "ok" : "FAIL", tiny, n);
    g_failures += trunc_ok ? 0 : 1;
    std::printf("%s: %d failure(s)\n", g_failures == 0 ? "PASS" : "FAIL", g_failures);
    return g_failures == 0 ? 0 : 1;
}

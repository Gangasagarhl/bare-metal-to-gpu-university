// rtc_decode.cpp - DR301 F4-04: host test of rtc_decode.h.
// (1) The same instant encoded four ways (BCD/binary x 24-hour/12-hour) decodes the same.
// (2) days_from_civil agrees with C++20 <chrono> for every day from 1900 to 2199.
#include <chrono>
#include <cstdio>
#include "rtc_decode.h"

int main()
{
    // 2026-10-09 20:45:30 as the clock chip would hold it in each mode
    const RtcRaw modes[] = {
        {0x30, 0x45, 0x20, 0x09, 0x10, 0x26, 0x20, RTC_B_24H},                 // BCD, 24 h
        {30, 45, 20, 9, 10, 26, 20, RTC_B_24H | RTC_B_BINARY},                  // binary, 24 h
        {0x30, 0x45, 0x80 | 0x08, 0x09, 0x10, 0x26, 0x20, 0},                    // BCD, 12 h, PM
        {30, 45, 0x80 | 8, 9, 10, 26, 20, RTC_B_BINARY},                         // binary, 12 h, PM
    };
    const char* names[] = {"BCD 24h", "binary 24h", "BCD 12h", "binary 12h"};
    int bad = 0;
    for (int i = 0; i < 4; ++i) {
        const DateTime t = rtc_decode(modes[i]);
        std::printf("%-10s hour reg 0x%02x -> %04d-%02d-%02d %02d:%02d:%02d unix %lld\n", names[i],
                    modes[i].hour, t.year, t.mon, t.day, t.hour, t.min, t.sec,
                    static_cast<long long>(to_unix(t)));
        if (to_unix(t) != to_unix(rtc_decode(modes[0]))) ++bad;
    }
    // midnight and noon in 12-hour mode: "12 AM" is hour 0, "12 PM" is hour 12
    const RtcRaw am12{0, 0, 0x12, 1, 1, 0x26, 0x20, 0}, pm12{0, 0, 0x80 | 0x12, 1, 1, 0x26, 0x20, 0};
    std::printf("12 AM -> hour %d, 12 PM -> hour %d\n", rtc_decode(am12).hour, rtc_decode(pm12).hour);
    if (rtc_decode(am12).hour != 0 || rtc_decode(pm12).hour != 12) ++bad;
    // the forensic mistake: BCD registers read as if they were binary
    const DateTime wrong = rtc_decode(modes[0], false);
    std::printf("BCD read as binary: %04d-%02d-%02d %02d:%02d:%02d\n", wrong.year, wrong.mon, wrong.day,
                wrong.hour, wrong.min, wrong.sec);

    using namespace std::chrono;
    long checked = 0;
    for (sys_days d = sys_days{year{1900} / 1 / 1}; d < sys_days{year{2200} / 1 / 1}; d += days{1}) {
        const year_month_day ymd{d};
        const int64_t ours = days_from_civil(int{ymd.year()}, static_cast<int>(unsigned{ymd.month()}),
                                             static_cast<int>(unsigned{ymd.day()}));
        if (ours != d.time_since_epoch().count()) {
            if (bad < 10) std::printf("MISMATCH at day %lld\n", static_cast<long long>(d.time_since_epoch().count()));
            ++bad;
        }
        ++checked;
    }
    std::printf("days_from_civil checked against <chrono> for %ld days (1900-01-01 .. 2199-12-31)\n", checked);
    static_assert(days_from_civil(1970, 1, 1) == 0);
    static_assert(days_from_civil(2000, 3, 1) == 11017);
    std::printf("%s\n", bad == 0 ? "all checks passed" : "CHECKS FAILED");
    return bad == 0 ? 0 : 1;
}

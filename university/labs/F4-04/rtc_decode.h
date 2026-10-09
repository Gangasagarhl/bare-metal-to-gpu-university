// rtc_decode.h - DR301 F4-04: turn a snapshot of the CMOS clock registers into a date,
// and a date into Unix time. Pure functions: used by the kernel and by the host test.
// Register meanings written from the author's memory of the Motorola MC146818 datasheet
// (register B: bit 2 = binary mode, bit 1 = 24-hour mode; hours bit 7 = PM in 12-hour
// mode); confirmed only by QEMU's RTC model and the host test (see F4-04).
#pragma once
#include <stdint.h>

struct RtcRaw {                  // the registers as read, before any conversion
    uint8_t sec, min, hour, day, mon, year, century, reg_b;
};

struct DateTime { int year, mon, day, hour, min, sec; };

constexpr uint8_t RTC_B_24H = 0x02, RTC_B_BINARY = 0x04;

constexpr int bcd(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }

constexpr DateTime rtc_decode(const RtcRaw& r, bool honour_binary_bit = true)
{
    const bool binary = honour_binary_bit ? (r.reg_b & RTC_B_BINARY) != 0 : true;
    auto conv = [binary](uint8_t v) { return binary ? int{v} : bcd(v); };
    DateTime t{};
    t.sec = conv(r.sec);
    t.min = conv(r.min);
    const bool pm = !(r.reg_b & RTC_B_24H) && (r.hour & 0x80);   // 12-hour mode: bit 7 = PM
    int h = conv(static_cast<uint8_t>(r.hour & 0x7F));
    if (!(r.reg_b & RTC_B_24H)) {                // 12 AM = 0 h, 12 PM = 12 h
        if (h == 12) h = 0;
        if (pm) h += 12;
    }
    t.hour = h;
    t.day = conv(r.day);
    t.mon = conv(r.mon);
    const int yy = conv(r.year);
    // no century register (0): assume years 2000-2099
    t.year = r.century ? conv(r.century) * 100 + yy : 2000 + yy;
    return t;
}

// Days since 1970-01-01 of a proleptic Gregorian date. Shift the year to start in March so
// the leap day is the last day of the "year", then count whole 400-year eras.
constexpr int64_t days_from_civil(int y, int m, int d)
{
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const int64_t yoe = y - era * 400;                                   // [0, 399]
    const int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;  // [0, 365]
    const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;           // [0, 146096]
    return era * 146097 + doe - 719468;
}

constexpr int64_t to_unix(const DateTime& t)
{
    return days_from_civil(t.year, t.mon, t.day) * 86400 + t.hour * 3600 + t.min * 60 + t.sec;
}

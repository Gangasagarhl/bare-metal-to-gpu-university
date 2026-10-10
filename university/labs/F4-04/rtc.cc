// rtc.cc - DR301 F4-04: read the MC146818-compatible CMOS clock without tearing.
// Index port 0x70, data port 0x71; registers 0x00 seconds, 0x02 minutes, 0x04 hours,
// 0x07 day of month, 0x08 month, 0x09 year, 0x0A status A (bit 7: update in progress),
// 0x0B status B (format bits). Written from the author's memory of the MC146818 datasheet
// and the OSDev wiki "CMOS" page; confirmed only by QEMU's RTC model (see F4-04).
#include "rtc.h"
#include "kbase.h"

namespace {
uint8_t g_century_reg = 0;
uint32_t g_retries = 0;

uint8_t cmos(uint8_t reg)
{
    outb(0x70, reg);                        // bit 7 of this port also gates NMI: left at 0
    return inb(0x71);
}

bool update_in_progress() { return cmos(0x0A) & 0x80; }

RtcRaw read_once()
{
    RtcRaw r{};
    r.sec = cmos(0x00);
    r.min = cmos(0x02);
    r.hour = cmos(0x04);
    r.day = cmos(0x07);
    r.mon = cmos(0x08);
    r.year = cmos(0x09);
    r.century = g_century_reg ? cmos(g_century_reg) : 0;
    r.reg_b = cmos(0x0B);
    return r;
}

bool same(const RtcRaw& a, const RtcRaw& b) { return memcmp(&a, &b, sizeof a) == 0; }
}  // namespace

namespace rtc {
void init(uint8_t century_register) { g_century_reg = century_register; }

RtcRaw read_raw()
{
    // The chip updates its registers once a second. A read that straddles the update can
    // mix old and new values (23:59:59 + 00:00:00 = 23:00:00 or 00:59:59). So: wait until
    // no update is in progress, read everything, and repeat until two snapshots agree.
    RtcRaw a, b;
    while (update_in_progress()) { }
    a = read_once();
    for (;;) {
        while (update_in_progress()) { }
        b = read_once();
        if (same(a, b)) return b;
        ++g_retries;
        a = b;
    }
}

DateTime now()
{
#ifdef F404_IGNORE_BINARY_BIT
    return rtc_decode(read_raw(), false);   // forensic build: assumes binary registers
#else
    return rtc_decode(read_raw());
#endif
}

uint32_t retries() { return g_retries; }
uint8_t read_register(uint8_t reg) { return cmos(reg); }
}  // namespace rtc

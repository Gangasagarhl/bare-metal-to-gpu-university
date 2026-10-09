// f404_main.cc - DR301 F4-04: wall-clock time from the RTC, joined to the monotonic tick.
#include "../F4-01/kbase.h"
#include "../F4-02/acpi.h"
#include "../F4-03/irq.h"
#include "rtc.h"

namespace {
void print_dt(const char* label, const DateTime& t)
{
    kprintf("%s %04d-%02d-%02d %02d:%02d:%02d UTC (unix %lu)\n", label, t.year, t.mon, t.day, t.hour,
            t.min, t.sec, static_cast<uint64_t>(to_unix(t)));
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t)
{
    serial_init();
    kprintf("F4-04 kernel: magic=0x%x\n", magic);
    irq::init();
    timer::init();
    irq::enable();

    // 1. Where is the century? The FADT (signature "FACP") names a CMOS register for it.
    uint8_t century = 0;
    if (acpi::init()) {
        if (const AcpiHeader* fadt = acpi::find("FACP"); fadt && fadt->length > 108)
            century = reinterpret_cast<const uint8_t*>(fadt)[108];
    }
    kprintf("FADT century register: 0x%02x%s\n", century, century ? "" : " (none: assume 20xx)");
    rtc::init(century);

    // 2. One consistent snapshot, raw and decoded.
    const RtcRaw r = rtc::read_raw();
    kprintf("raw: sec=%02x min=%02x hour=%02x day=%02x mon=%02x year=%02x century=%02x B=%02x (%s, %s)\n",
            r.sec, r.min, r.hour, r.day, r.mon, r.year, r.century, r.reg_b,
            (r.reg_b & RTC_B_BINARY) ? "binary" : "BCD", (r.reg_b & RTC_B_24H) ? "24-hour" : "12-hour");
    const DateTime t0 = rtc::now();
    print_dt("date:", t0);

    // 3. Join wall-clock time to the monotonic clock at a second boundary: wait for the
    //    seconds register to change, then remember (unix time, tick count) as one pair.
    int last = t0.sec;
    DateTime t = t0;
    uint64_t edge_ms = 0;
    int64_t edge_unix = 0;
    for (int edges = 0; edges < 4;) {
        t = rtc::now();
        if (t.sec == last) { irq::wait(); continue; }
        const uint64_t ms = timer::ms();
        if (edges > 0) kprintf("  second edge: %02d:%02d:%02d, %u ms of PIT ticks since the previous edge\n",
                               t.hour, t.min, t.sec, static_cast<uint32_t>(ms - edge_ms));
        else print_dt("  first edge:", t);
        edge_ms = ms;
        edge_unix = to_unix(t);
        last = t.sec;
        ++edges;
    }
    print_dt("date:", t);

    // 4. From now on, wall time = edge_unix + (ticks since the edge) / 1000, with no RTC reads.
    timer::sleep_ms(1500);
    const uint64_t since = timer::ms() - edge_ms;
    const int64_t wall = edge_unix + static_cast<int64_t>(since / 1000);
    const DateTime check = rtc::now();
    kprintf("monotonic-based wall time %lu.%03u; RTC says %lu; snapshot retries %u\n",
            static_cast<uint64_t>(wall), static_cast<uint32_t>(since % 1000),
            static_cast<uint64_t>(to_unix(check)), rtc::retries());
    kprintf("KERNEL_UNIX %lu\n", static_cast<uint64_t>(to_unix(t0)));
    kprintf("F4-04 done\n");
    qemu_exit(0x10);
}

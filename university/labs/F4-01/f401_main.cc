// f401_main.cc - DR301 F4-01: the lab kernel's platform bus, two more small drivers, and
// the binding run that the chapter discusses. Boots under QEMU with -kernel.
#include "driver.h"
#include "kbase.h"

extern Driver uart16550_driver;
void uart_write(Device& d, const char* s);
void uart_dump(Device& d);

namespace {
// i8042 keyboard controller: command 0xAA asks for a self-test, 0x55 means "passed".
int i8042_probe(Device& d)
{
    const uint16_t data = d.io_base, cmd = static_cast<uint16_t>(d.io_base + 4);
    while (inb(cmd) & 0x01) (void)inb(data);           // status bit 0: output buffer full
    while (inb(cmd) & 0x02) { }                        // status bit 1: input buffer full
    outb(cmd, 0xAA);
    for (int i = 0; i < 1000000; ++i)
        if (inb(cmd) & 0x01) return inb(data) == 0x55 ? 0 : -E_IO;
    return -E_TIMEDOUT;
}
constexpr MatchId i8042_ids[] = {{BusKind::Platform, "i8042", 0xFFFF, 0xFFFF, 0, 0}};
Driver i8042_driver = {"i8042", i8042_ids, 1, i8042_probe, nullptr};

// CMOS real-time clock: register 0x0D bit 7 reports "valid RAM and time".
int rtc_probe(Device& d)
{
    outb(d.io_base, 0x0D);
    return (inb(static_cast<uint16_t>(d.io_base + 1)) & 0x80) ? 0 : -E_NODEV;
}
void rtc_remove(Device&) { kprintf("  rtc-cmos: remove called\n"); }
constexpr MatchId rtc_ids[] = {{BusKind::Platform, "mc146818", 0xFFFF, 0xFFFF, 0, 0}};
Driver rtc_driver = {"rtc-cmos", rtc_ids, 1, rtc_probe, rtc_remove};

Device platform(const char* name, const char* compatible, uint16_t io, uint16_t len, uint8_t irq)
{
    Device d{};
    d.name = name;
    d.bus = BusKind::Platform;
    d.compatible = compatible;
    d.vendor = d.device = 0xFFFF;
    d.io_base = io;
    d.io_len = len;
    d.irq = irq;
    return d;
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t /*mbi*/)
{
    serial_init();
    kprintf("F4-01 kernel: magic=0x%x\n", magic);

    // 1. The "bus" reports what is at the fixed legacy addresses of a PC.
    //    Nothing here proves that a device is really present: that is probe's job.
    dm::add_device(platform("com1", "ns16550", 0x3F8, 8, 4));
    dm::add_device(platform("com2", "ns16550", 0x2F8, 8, 3));
    dm::add_device(platform("i8042", "i8042", 0x60, 5, 1));
    dm::add_device(platform("rtc", "mc146818", 0x70, 2, 8));
    dm::add_device(platform("lpt1", "pc-parallel", 0x378, 3, 7));

    // 2. Drivers announce what they can handle; 3. the model binds them.
    dm::register_driver(uart16550_driver);
    dm::register_driver(i8042_driver);
    dm::register_driver(rtc_driver);
    dm::bind_all();
    dm::print_table();

    // 4. Use the bound UARTs through the driver's service, not through raw ports.
    for (size_t i = 0; i < dm::device_count(); ++i) {
        Device& d = dm::device_at(i);
        if (d.driver == &uart16550_driver) {
            uart_dump(d);
            kprintf("writing a test line through %s\n", d.name);
            uart_write(d, "  hello from the uart16550 driver\n");
        }
    }

    // 5. Lifecycle: remove the RTC driver from its device, then bind again.
    Device& rtc = dm::device_at(3);
    dm::unbind(rtc);
    kprintf("after unbind: rtc driver=%s\n", rtc.driver ? rtc.driver->name : "(none)");
    dm::bind_all();
    kprintf("after bind_all: rtc driver=%s\n", rtc.driver ? rtc.driver->name : "(none)");
    kprintf("F4-01 done\n");
    qemu_exit(0x10);                     // QEMU exit status 33 = pass
}

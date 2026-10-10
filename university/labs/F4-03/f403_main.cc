// f403_main.cc - DR301 F4-03: two tests of interrupt-driven input.
//   "keyboard": PS/2 keyboard into a line discipline, PS/2 mouse packets (milestone C2, test 1)
//   "paste":    64 KiB through COM2 with receive and transmit interrupts (C2, test 2)
#include "../F4-01/kbase.h"
#include "i8042.h"
#include "irq.h"
#include "tty.h"
#include "uart_irq.h"

namespace {
bool cmdline_has(uint32_t mbi, const char* word)
{
    const auto* info = reinterpret_cast<const uint32_t*>(mbi);
    if (!(info[0] & 0x4)) return false;          // flag bit 2: command line present
    const char* s = reinterpret_cast<const char*>(info[4]);
    const size_t n = kstrlen(word);
    for (; *s; ++s)
        if (memcmp(s, word, n) == 0) return true;
    return false;
}

void echo_to_log(char c)
{
    if (c == '\b') kputs("^H");                  // make the erase visible in the log
    else serial_putc(c);
}

void status_line(uint32_t keys, uint32_t packets)
{
    kprintf("status t=%u ms: irq0=%u irq1=%u irq12=%u keys=%u mouse_packets=%u\n",
            static_cast<uint32_t>(timer::ms()), irq::count(0), irq::count(1), irq::count(12), keys, packets);
}

void keyboard_test()
{
    if (ps2::init() != 0) panic("i8042 init failed");
    LineDiscipline tty(echo_to_log);
    uint32_t lines = 0, keys = 0, packets = 0;
    uint64_t next_status = 500;
    uint32_t keys_at_status = 0;
    kprintf("tty ready\n");
    while (lines < 2 || packets < 3) {
        KeyEvent k;
        while (ps2::get_key(k)) {
            if (!k.pressed) continue;
            ++keys;
            if (k.ascii && tty.feed(k.ascii)) {
                ++lines;
                kprintf("shell: line \"%s\" (%u chars)\n", tty.line(), static_cast<uint32_t>(kstrlen(tty.line())));
            }
        }
        MouseEvent m;
        while (ps2::get_mouse(m)) {
            ++packets;
            kprintf("mouse: dx=%d dy=%d buttons=%c%c%c\n", m.dx, m.dy, (m.buttons & 1) ? 'L' : '-',
                    (m.buttons & 4) ? 'M' : '-', (m.buttons & 2) ? 'R' : '-');
        }
        if (timer::ms() >= next_status) {        // a status line, but only while input is quiet
            if (keys == keys_at_status) { kputs("\n"); status_line(keys, packets); }
            keys_at_status = keys;
            next_status += 1000;
        }
        irq::wait();                             // sleep until the next interrupt
    }
    status_line(keys, packets);
    kprintf("mouse resyncs: %u\n", ps2::resyncs());
}

void paste_test()
{
    constexpr uint32_t expected = 65536;
    uart_irq_init(0x2F8, 3);                     // COM2 on line 3
    kprintf("paste: waiting for %u bytes on COM2\n", expected);
    uint32_t got = 0, h = 2166136261u;
    const uint64_t t0 = timer::ms();
    uint64_t last = t0;                          // time of the last byte (or of the start)
    bool reported = false;
    while (got < expected) {
        uint8_t b;
        while (got < expected && uart_irq_getc(b)) {
            h = fnv1a(&b, 1, h);
            ++got;
            uart_irq_putc(b);                    // echo back through the transmit ring
            last = timer::ms();
        }
        if (!reported && timer::ms() - last > 10000) {   // a stall: say what the hardware shows
            uint8_t ier, lsr, mcr;
            uart_irq_regs(ier, lsr, mcr);
            kprintf("paste: no byte for 10000 ms after %u bytes; IER=0x%02x LSR=0x%02x MCR=0x%02x irq3=%u irq0=%u\n",
                    got, ier, lsr, mcr, irq::count(3), irq::count(0));
            reported = true;
        }
        irq::wait();
    }
    while (!uart_irq_tx_idle()) irq::wait();
    const UartStats& s = uart_irq_stats();
    kprintf("paste: received %u bytes in %u ms (emulated time)\n", got, static_cast<uint32_t>(timer::ms() - t0));
    kprintf("paste: fnv1a %08x\n", h);
    kprintf("paste: interrupts %u, rx %u, tx %u, overruns %u, ring drops %u, ring high-water %u of 4096\n",
            s.interrupts, s.rx_bytes, s.tx_bytes, s.overruns, s.rx_dropped, s.rx_high_water);
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t mbi)
{
    serial_init();
    kprintf("F4-03 kernel: magic=0x%x\n", magic);
    irq::init();
    timer::init();
    irq::enable();
    if (cmdline_has(mbi, "paste")) paste_test();
    else keyboard_test();
    kprintf("F4-03 done\n");
    qemu_exit(0x10);
}

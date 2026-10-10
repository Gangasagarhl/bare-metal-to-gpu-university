// irq.cc - DR301 F4-03: IDT setup, 8259 PIC remapping, dispatch, PIT tick.
// 8259 and 8254 programming written from the author's memory of the Intel 8259A and
// 8254 datasheets; checked only by QEMU's i8259 and i8254 models behaving as expected.
#include "irq.h"
#include "kbase.h"

extern "C" char isr_stubs[];             // 48 stubs, 16 bytes apart (isr.S)

namespace {
struct Gate {                            // 32-bit interrupt gate
    uint16_t offset_lo, selector;
    uint8_t zero, type_attr;             // 0x8E: present, ring 0, 32-bit interrupt gate
    uint16_t offset_hi;
} __attribute__((packed));
static_assert(sizeof(Gate) == 8, "IDT gates are 8 bytes in protected mode");

Gate g_idt[48];
IrqHandler g_handlers[16];
volatile uint32_t g_counts[16];
volatile uint64_t g_ms;

constexpr uint16_t PIC1_CMD = 0x20, PIC1_DATA = 0x21, PIC2_CMD = 0xA0, PIC2_DATA = 0xA1;
constexpr uint8_t PIC_EOI = 0x20;

void io_wait() { outb(0x80, 0); }        // a write to an unused port: a short delay

void pic_remap()
{
    outb(PIC1_CMD, 0x11); io_wait();     // ICW1: initialise, ICW4 follows
    outb(PIC2_CMD, 0x11); io_wait();
    outb(PIC1_DATA, 32); io_wait();      // ICW2: lines 0-7 -> vectors 32-39
    outb(PIC2_DATA, 40); io_wait();      //       lines 8-15 -> vectors 40-47
    outb(PIC1_DATA, 0x04); io_wait();    // ICW3: slave on master line 2
    outb(PIC2_DATA, 0x02); io_wait();    //       slave's cascade identity 2
    outb(PIC1_DATA, 0x01); io_wait();    // ICW4: 8086 mode
    outb(PIC2_DATA, 0x01); io_wait();
    outb(PIC1_DATA, 0xFB);               // mask everything except the cascade (line 2)
    outb(PIC2_DATA, 0xFF);
}

void timer_tick() { g_ms = g_ms + 1; }
}  // namespace

extern "C" void interrupt_dispatch(IrqFrame* f)
{
    if (f->vector < 32) {
        kprintf("EXCEPTION %u error 0x%x at eip 0x%x\n", f->vector, f->error_code, f->eip);
        panic("CPU exception");
    }
    const uint8_t line = static_cast<uint8_t>(f->vector - 32);
#ifndef F403_FORGET_EOI_LINE
    constexpr int forget = -1;
#else
    constexpr int forget = F403_FORGET_EOI_LINE;   // forensic build: one line never gets EOI
#endif
    g_counts[line] = g_counts[line] + 1;
    if (g_handlers[line]) g_handlers[line]();
    // End of interrupt: the PIC will not deliver this line (or lower-priority lines) again
    // until it is told the handler is done. Lines 8-15 need an EOI at both PICs.
    if (line != forget) {
        if (line >= 8) outb(PIC2_CMD, PIC_EOI);
        outb(PIC1_CMD, PIC_EOI);
    }
}

namespace irq {
void init()
{
    for (int v = 0; v < 48; ++v) {
        const auto addr = reinterpret_cast<uintptr_t>(isr_stubs) + 16u * v;
        g_idt[v] = Gate{static_cast<uint16_t>(addr & 0xFFFF), 0x08, 0, 0x8E,
                        static_cast<uint16_t>(addr >> 16)};
    }
    struct { uint16_t limit; uint32_t base; } __attribute__((packed)) idtr{sizeof(g_idt) - 1,
                                                    reinterpret_cast<uintptr_t>(g_idt)};
    asm volatile("lidt %0" : : "m"(idtr));
    pic_remap();
}

void set_handler(uint8_t line, IrqHandler h) { g_handlers[line] = h; }

void unmask(uint8_t line)
{
    const uint16_t port = line < 8 ? PIC1_DATA : PIC2_DATA;
    outb(port, inb(port) & ~(1u << (line & 7)));
}

void mask(uint8_t line)
{
    const uint16_t port = line < 8 ? PIC1_DATA : PIC2_DATA;
    outb(port, inb(port) | (1u << (line & 7)));
}

uint32_t count(uint8_t line) { return g_counts[line]; }
}  // namespace irq

namespace timer {
void init()
{
    // PIT channel 0, mode 2 (rate generator), divisor 1193: about 1000 interrupts per
    // second from the 8254's input clock of about 1.193182 MHz (from memory; see F4-03).
    constexpr uint16_t divisor = 1193;
    outb(0x43, 0x34);                    // channel 0, low byte then high byte, mode 2
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
    irq::set_handler(0, timer_tick);
    irq::unmask(0);
}

uint64_t ms()
{
    // a 64-bit read is two 32-bit loads on this CPU: read until two reads agree
    uint64_t a, b;
    do { a = g_ms; b = g_ms; } while (a != b);
    return a;
}

void sleep_ms(uint32_t n)
{
    const uint64_t end = ms() + n;
    while (ms() < end) irq::wait();
}
}  // namespace timer

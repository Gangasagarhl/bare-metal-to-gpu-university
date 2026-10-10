// intr.cc - IDT set-up, exception reports, the 8259 PIC and the 8254 PIT.
// Port numbers and command bytes are the classic PC ones (Intel 8259A and 8254 data
// sheets, title only, not opened in this build); the runs prove them on QEMU's models.
#include "intr.h"
#include "kio.h"

extern "C" const uint64_t isr_table[33];

namespace {
struct IdtEntry {
    uint16_t off0, sel;
    uint8_t ist, type;
    uint16_t off1;
    uint32_t off2, zero;
} __attribute__((packed));
IdtEntry g_idt[256];
volatile uint64_t g_ticks;
uint8_t g_pic_base;
}

extern "C" void isr_dispatch(Frame* f)
{
    if (f->vector == 32) {
        g_ticks = g_ticks + 1;
        outb(0x20, 0x20);   // end of interrupt to the master PIC
        return;
    }
    kprintf("EXCEPTION vector %lu error %lx at rip %lx\n", f->vector, f->error, f->rip);
    qemu_exit(1);
}

void idt_init()
{
    for (unsigned v = 0; v < 33; ++v) {
        uint64_t a = isr_table[v];
        g_idt[v] = IdtEntry{static_cast<uint16_t>(a), 0x08, 0, 0x8E,   // present, ring 0,
                            static_cast<uint16_t>(a >> 16),             // interrupt gate
                            static_cast<uint32_t>(a >> 32), 0};
    }
    struct { uint16_t limit; uint64_t base; } __attribute__((packed)) idtr{
        sizeof g_idt - 1, reinterpret_cast<uint64_t>(g_idt)};
    asm volatile("lidt %0" : : "m"(idtr));
}

void timer_start(unsigned hz)
{
    g_pic_base = 32;
    outb(0x20, 0x11); outb(0xA0, 0x11);           // ICW1: init, ICW4 follows
    outb(0x21, g_pic_base); outb(0xA1, g_pic_base + 8);   // ICW2: vector bases
    outb(0x21, 0x04); outb(0xA1, 0x02);           // ICW3: slave on IRQ 2
    outb(0x21, 0x01); outb(0xA1, 0x01);           // ICW4: 8086 mode
    outb(0x21, 0xFE); outb(0xA1, 0xFF);           // mask all but IRQ 0
    uint32_t divisor = 1193182u / hz;             // PIT input clock / tick rate
    outb(0x43, 0x34);                             // channel 0, lo/hi byte, mode 2
    outb(0x40, static_cast<uint8_t>(divisor));
    outb(0x40, static_cast<uint8_t>(divisor >> 8));
}

uint64_t ticks() { return g_ticks; }

uint16_t pit_read_counter()
{
    outb(0x43, 0x00);                             // latch channel 0
    uint8_t lo = inb(0x40);
    uint8_t hi = inb(0x40);
    return static_cast<uint16_t>(lo | (hi << 8));
}

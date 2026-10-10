// translate_demo.cc - BR-07 trap 2: a bus address used as if it were a CPU address.
//
// Runs on QEMU's raspi3b model with the devicetree from F4-27's make_dtb.py (-dtb pi3.dtb).
// It finds the console node /soc/serial@7e201000 with F4-23's fdt.h, translates its reg
// through /soc's ranges, prints through the translated (CPU) address, and then makes the
// classic mistake: it writes to the untranslated bus address. What happens next is the
// evidence. Untested on hardware: what a real Pi does with that write is not known here.
#include <stddef.h>
#include <stdint.h>

#include "../F4-23/fdt.h"

namespace {

uint64_t g_uart = 0;                 // CPU address of the PL011, once known

void wr32(uint64_t a, uint32_t v) { *reinterpret_cast<volatile uint32_t*>(a) = v; }
uint32_t rd32(uint64_t a) { return *reinterpret_cast<volatile uint32_t*>(a); }

void putc(char c)
{
    while ((rd32(g_uart + 0x18) & (1u << 5)) != 0) {   // flag register: transmit FIFO full
    }
    wr32(g_uart + 0x00, static_cast<uint8_t>(c));      // data register
}

void puts(const char* s)
{
    for (; *s != '\0'; ++s) {
        if (*s == '\n') {
            putc('\r');
        }
        putc(*s);
    }
}

void puthex(uint64_t v)
{
    puts("0x");
    bool started = false;
    for (int shift = 60; shift >= 0; shift -= 4) {
        unsigned d = static_cast<unsigned>(v >> shift) & 0xf;
        if (d != 0 || started || shift == 0) {
            putc(static_cast<char>(d < 10 ? '0' + d : 'a' + d - 10));
            started = true;
        }
    }
}

// Semihosting SYS_EXIT through HLT #0xF000 (QEMU -semihosting), as in DR402's labs.
[[noreturn]] void qemu_exit(int code)
{
    static uint64_t block[2];
    block[0] = 0x20026;
    block[1] = static_cast<uint64_t>(code);
    register uint64_t x0 asm("x0") = 0x18;
    register uint64_t x1 asm("x1") = reinterpret_cast<uint64_t>(block);
    asm volatile("hlt #0xf000" : : "r"(x0), "r"(x1) : "memory");
    while (true) {
    }
}

}  // namespace

extern "C" [[noreturn]] void on_exception(uint64_t vec, uint64_t esr, uint64_t elr, uint64_t far)
{
    puts("EXCEPTION vector ");
    puthex(vec);
    puts(": ESR_EL1=");
    puthex(esr);
    puts(" (class ");
    puthex((esr >> 26) & 0x3f);
    puts(", fault status ");
    puthex(esr & 0x3f);
    puts(") FAR_EL1=");
    puthex(far);
    puts(" ELR_EL1=");
    puthex(elr);
    puts("\n");
    qemu_exit(3);
}

extern "C" [[noreturn]] void kmain(const void* dtb)
{
    fdt::Blob t;
    fdt::Node soc;
    fdt::Node uart;
    fdt::Prop ranges;
    uint64_t bus = 0;
    uint64_t size = 0;
    if (!t.init(dtb) || !t.find_path("/soc", &soc) || !t.find_path("/soc/serial", &uart) ||
        !t.reg(uart, 0, &bus, &size) || !t.get_prop(soc, "ranges", &ranges) || ranges.len != 12) {
        qemu_exit(2);                // no console to complain on: the exit code says it
    }
    // /soc has #address-cells = 1 and #size-cells = 1, its parent 1: one triple per window.
    uint64_t child = fdt::be32(ranges.data);
    uint64_t parent = fdt::be32(ranges.data + 4);
    uint64_t len = fdt::be32(ranges.data + 8);
    if (bus < child || bus - child >= len) {
        qemu_exit(4);
    }
    uint64_t cpu = parent + (bus - child);
    g_uart = cpu;
    puts("devicetree: /soc/serial reg = ");
    puthex(bus);
    puts(" (a bus address)\n/soc ranges: child ");
    puthex(child);
    puts(" -> parent ");
    puthex(parent);
    puts(", length ");
    puthex(len);
    puts("\ntranslated CPU address = ");
    puthex(cpu);
    puts("; this line was written through it\n");
    puts("now writing 'X' to the untranslated address ");
    puthex(bus);
    puts(" ...\n");
    wr32(bus, 'X');                  // the trap
    puts("the write returned without an exception\n");
    qemu_exit(0);
}

// The compiler may emit calls to these for zeroing or copying objects.
extern "C" void* memset(void* d, int c, size_t n)
{
    auto* p = static_cast<unsigned char*>(d);
    for (size_t i = 0; i < n; ++i) {
        p[i] = static_cast<unsigned char>(c);
    }
    return d;
}
extern "C" void* memcpy(void* d, const void* s, size_t n)
{
    auto* p = static_cast<unsigned char*>(d);
    const auto* q = static_cast<const unsigned char*>(s);
    for (size_t i = 0; i < n; ++i) {
        p[i] = q[i];
    }
    return d;
}

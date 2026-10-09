// kernel.cc - the OS301 kernel stub: the first code that owns the machine (milestone A3).
// Freestanding C++: no standard library, no exceptions, no RTTI. It checks the boot
// information, reports it on the serial port (COM1, I/O port 0x3F8, already set up by the
// firmware), paints a pattern on the framebuffer, and halts.
#include "bootinfo.hpp"

namespace {

void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void putc(char c)
{
    while ((inb(0x3f8 + 5) & 0x20) == 0) {  // wait until the transmitter can take a byte
    }
    outb(0x3f8, static_cast<uint8_t>(c));
}

void print(const char* s)
{
    for (; *s != '\0'; ++s) {
        if (*s == '\n') {
            putc('\r');
        }
        putc(*s);
    }
}

void dec(uint64_t v)
{
    char buf[21];
    int i = 20;
    buf[i] = '\0';
    do {
        buf[--i] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v != 0);
    print(&buf[i]);
}

void hex(uint64_t v)
{
    char buf[19] = "0x";
    for (int i = 15; i >= 0; --i) {
        buf[2 + i] = "0123456789abcdef"[v & 0xf];
        v >>= 4;
    }
    buf[18] = '\0';
    print(buf);
}

uint64_t rdtsc()
{
    uint32_t lo = 0, hi = 0;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return static_cast<uint64_t>(hi) << 32 | lo;
}

[[noreturn]] void halt(const char* why)
{
    print(why);
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

}  // namespace

// .data and .bss on purpose (volatile, so the compiler keeps them in memory): the loader must
// copy the first from the file and fill the second with zeros.
volatile uint32_t pattern = 0x00336699;  // 0x00RRGGBB in the usual 32-bit pixel formats
volatile uint64_t bss_word;

extern "C" [[noreturn]] void kernel_main(const os301::BootInfo* bi)
{
    const uint64_t tsc_entry = rdtsc();
    print("kernel entered\n");
    if (bi->magic != os301::kBootMagic) {
        halt("kernel: boot info magic is wrong, halting\n");
    }
    print("boot info version ");
    dec(bi->version);
    print(", size ");
    dec(bi->size);
    print(" bytes, kernel built for version ");
    dec(os301::kBootInfoVersion);
    print("\n");
    if (bi->version < os301::kBootInfoVersion) {
        halt("kernel: loader too old for this kernel, halting\n");
    }
    print(".bss word is ");
    dec(bss_word);
    print(" (must be 0), .data pattern is ");
    hex(pattern);
    print("\n");

    const auto* rsdp = reinterpret_cast<const char*>(bi->direct_map_base + bi->rsdp_physical);
    char sig[9] = {};
    for (int i = 0; i < 8; ++i) {
        sig[i] = rsdp[i];
    }
    print("RSDP at physical ");
    hex(bi->rsdp_physical);
    print(": signature '");
    print(sig);
    print("', revision ");
    dec(static_cast<uint8_t>(rsdp[15]));
    print("\n");

    const auto* map = reinterpret_cast<const os301::MemoryRange*>(bi->memory_map);
    uint64_t usable = 0;
    for (uint64_t i = 0; i < bi->memory_map_count; ++i) {
        if (map[i].type >= 1 && map[i].type <= 4) {   // loader and boot-services memory
            usable += map[i].pages;
        } else if (map[i].type == 7) {                // conventional memory
            usable += map[i].pages;
        }
    }
    print("memory map: ");
    dec(bi->memory_map_count);
    print(" entries, usable ");
    dec(usable * 4);
    print(" KiB\n");

    print("framebuffer: ");
    dec(bi->fb_width);
    print(" x ");
    dec(bi->fb_height);
    print(", pitch ");
    dec(bi->fb_pitch);
    print(" bytes, format ");
    dec(bi->fb_format);
    print(", at physical ");
    hex(bi->fb_physical);
    print(", size ");
    dec(bi->fb_size);
    print(" bytes\n");
    print("kernel: physical ");
    hex(bi->kernel_physical);
    print(", virtual ");
    hex(bi->kernel_virtual);
    print(", ");
    dec(bi->kernel_size);
    print(" bytes\ncommand line: '");
    print(bi->command_line);
    print("'\n");

    print("boot timeline (time-stamp counter ticks since the loader started):\n");
    print("  kernel file read         +");
    dec(bi->tsc_kernel_file_read - bi->tsc_loader_entry);
    print("\n  before ExitBootServices  +");
    dec(bi->tsc_before_exit - bi->tsc_loader_entry);
    print("\n  after ExitBootServices   +");
    dec(bi->tsc_after_exit - bi->tsc_loader_entry);
    print("  (");
    dec(bi->exit_attempts);
    print(" attempt(s))\n  kernel entered           +");
    dec(tsc_entry - bi->tsc_loader_entry);
    print("\n");

    // the known pattern: the top 64 rows of the screen, through the direct map
    auto* fb = reinterpret_cast<volatile uint32_t*>(bi->direct_map_base + bi->fb_physical);
    for (uint32_t y = 0; y < 64 && y < bi->fb_height; ++y) {
        for (uint32_t x = 0; x < bi->fb_width; ++x) {
            fb[y * (bi->fb_pitch / 4) + x] = pattern;
        }
    }
    print("framebuffer: top 64 rows painted with ");
    hex(pattern);
    print("\n");
    halt("kernel: halting (cli; hlt)\n");
}

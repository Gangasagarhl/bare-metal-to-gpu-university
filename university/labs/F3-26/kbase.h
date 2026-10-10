// kbase.h - OS303 teaching kernel: CPU helpers, console, panic, memory interfaces.
// Freestanding C++20: no exceptions, no RTTI, no standard library beyond the
// freestanding headers (<stdint.h>, <stddef.h>, <stdarg.h>, <atomic>).
#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

namespace k {

// ---- single instructions (Intel SDM Vol. 2 instruction reference; title only) ----
inline void outb(uint16_t port, uint8_t v) { asm volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
inline uint8_t inb(uint16_t port)
{
    uint8_t v;
    asm volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return (uint64_t(hi) << 32) | lo;
}
inline void wrmsr(uint32_t msr, uint64_t v)
{
    asm volatile("wrmsr" : : "c"(msr), "a"(uint32_t(v)), "d"(uint32_t(v >> 32)));
}
inline uint64_t read_cr2() { uint64_t v; asm volatile("mov %%cr2, %0" : "=r"(v)); return v; }
inline uint64_t read_cr3() { uint64_t v; asm volatile("mov %%cr3, %0" : "=r"(v)); return v; }
inline void write_cr3(uint64_t v) { asm volatile("mov %0, %%cr3" : : "r"(v) : "memory"); }
inline uint64_t read_cr4() { uint64_t v; asm volatile("mov %%cr4, %0" : "=r"(v)); return v; }
inline void write_cr4(uint64_t v) { asm volatile("mov %0, %%cr4" : : "r"(v) : "memory"); }
inline void invlpg(uint64_t va) { asm volatile("invlpg (%0)" : : "r"(va) : "memory"); }
inline void cpu_relax() { asm volatile("pause" ::: "memory"); }
inline uint64_t rdtsc()
{
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (uint64_t(hi) << 32) | lo;
}
inline void cpuid(uint32_t leaf, uint32_t sub, uint32_t r[4])
{
    asm volatile("cpuid" : "=a"(r[0]), "=b"(r[1]), "=c"(r[2]), "=d"(r[3]) : "a"(leaf), "c"(sub));
}

// Interrupt flag (RFLAGS.IF, bit 9). irq_save() returns the old RFLAGS and disables.
inline uint64_t irq_save()
{
    uint64_t f;
    asm volatile("pushfq; popq %0; cli" : "=r"(f) : : "memory");
    return f;
}
inline void irq_restore(uint64_t f)
{
    if (f & (1u << 9)) {
        asm volatile("sti" ::: "memory");
    }
}
inline bool irqs_enabled()
{
    uint64_t f;
    asm volatile("pushfq; popq %0" : "=r"(f));
    return (f & (1u << 9)) != 0;
}
inline void irq_enable() { asm volatile("sti" ::: "memory"); }
inline void irq_disable() { asm volatile("cli" ::: "memory"); }

// ---- console (COM1 16550 UART) and test exit (QEMU isa-debug-exit) ----
void console_init();
void kprintf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void kvprintf(const char* fmt, va_list ap);
void console_set_quiet(bool quiet);   // user-program output is dropped while quiet
bool console_quiet();
void console_write_raw(const char* s, size_t n);
void console_panic_mode();           // from now on print without taking the console lock
[[noreturn]] void panic(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
[[noreturn]] void qemu_exit(int code);  // QEMU exit status becomes (code << 1) | 1
void str_copy(char* dst, const char* src, size_t cap);
bool str_eq(const char* a, const char* b);
size_t str_len(const char* s);

#define KASSERT(c)                                                     \
    do {                                                               \
        if (!(c)) ::k::panic("assertion failed: %s (%s:%d)", #c, __FILE__, __LINE__); \
    } while (0)

// ---- physical frames (mem.cc) ----
constexpr uint64_t kPage = 4096;
void mem_init(uint64_t mb_info_addr);
uint64_t frame_alloc();             // returns a zeroed 4 KiB frame (physical = virtual here)
void frame_free(uint64_t pa);
uint64_t frames_free();
uint64_t frames_total();

// ---- kernel heap: size classes 16..2048 bytes, one frame per slab (mem.cc) ----
void* kmalloc(size_t n);
void kfree(void* p);
uint64_t heap_live_objects();

// ---- kernel page mapping with 4 KiB pages above 4 GiB (mem.cc) ----
// Kernel-only mappings shared by every address space (PML4 entry 0).
constexpr uint64_t kStackArea = 0x100000000ull;   // 4 GiB: thread stacks with guard pages
constexpr uint64_t kProbeArea = 0x140000000ull;   // 5 GiB: pages for TLB tests
void kmap_page(uint64_t va, uint64_t pa);
uint64_t kunmap_page(uint64_t va);                 // returns the frame, no TLB flush
bool kpage_mapped(uint64_t va);

// ---- boot information copied from the Multiboot structure (mem.cc) ----
struct BootModule {
    char name[32];
    const uint8_t* data;
    size_t size;
};
const char* boot_cmdline();
const char* boot_arg(const char* key);   // value of key=value on the command line, or nullptr
int boot_module_count();
const BootModule* boot_module(int i);
const BootModule* boot_module_find(const char* name);

}  // namespace k

// The compiler may emit calls to these four even in freestanding code.
extern "C" void* memset(void* d, int c, size_t n);
extern "C" void* memcpy(void* d, const void* s, size_t n);
extern "C" void* memmove(void* d, const void* s, size_t n);
extern "C" int memcmp(const void* a, const void* b, size_t n);

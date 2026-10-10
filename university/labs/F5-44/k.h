// k.h - DS403 cluster kernel: the services every other file needs.
// Freestanding C++: no exceptions, no RTTI, no standard library, no heap, no interrupts.
#pragma once
#include <stdint.h>
#include <stddef.h>

inline void outb(uint16_t port, uint8_t v) { asm volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
inline uint8_t inb(uint16_t port)
{
    uint8_t v;
    asm volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
inline void outl(uint16_t port, uint32_t v) { asm volatile("outl %0, %1" : : "a"(v), "Nd"(port)); }
inline uint32_t inl(uint16_t port)
{
    uint32_t v;
    asm volatile("inl %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

void serial_init();
void kprintf(const char* fmt, ...);        // %s %c %d %u %x, optional width and '0' flag
void klog(const char* fmt, ...);           // "[t=000123 n2] " + message + newline
[[noreturn]] void qemu_exit(uint8_t code); // isa-debug-exit: QEMU exits with (code << 1) | 1

// Time: ticks of 10 ms counted by polling the PIT (no interrupts).
void clock_init();
uint32_t now();                            // ticks since clock_init

// The Multiboot command line, for example "node=2 role=client".
void cmdline_init(uint32_t magic, uint32_t mbinfo);
uint32_t arg_u32(const char* key, uint32_t fallback);
bool arg_is(const char* key, const char* value);
const char* boot_loader_name();            // Multiboot field, "" if the loader gave none

// Pseudo-random numbers (xorshift32), seeded per node so timeouts differ.
void rand_seed(uint32_t s);
uint32_t rand_u32();

extern "C" void* memcpy(void* d, const void* s, size_t n);
extern "C" void* memset(void* d, int c, size_t n);
extern "C" int memcmp(const void* a, const void* b, size_t n);
size_t kstrlen(const char* s);
int kstrcmp(const char* a, const char* b);
void kstrlcpy(char* d, const char* s, size_t cap);

extern uint8_t g_node;                     // this node's id (1..8), from "node="

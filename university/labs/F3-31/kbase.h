// kbase.h - F3-31 mini kernel: the few services every other file needs.
// Freestanding C++: no exceptions, no RTTI, no standard library, no heap.
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

void serial_init();
void serial_putc(char c);
int serial_getc();                       // blocks; returns -1 never (polls forever)
void kputs(const char* s);
void kprintf(const char* fmt, ...);      // %s %c %d %u %x %o, optional width and '0' flag
[[noreturn]] void qemu_exit(uint8_t code); // isa-debug-exit: QEMU exits with (code << 1) | 1

extern "C" void* memcpy(void* d, const void* s, size_t n);
extern "C" void* memset(void* d, int c, size_t n);
size_t kstrlen(const char* s);
int kstrcmp(const char* a, const char* b);
int kstrncmp(const char* a, const char* b, size_t n);
void kstrlcpy(char* d, const char* s, size_t cap);

// Error numbers returned as negative values, after the POSIX names (values are ours).
enum : int { E_OK = 0, E_NOENT = 2, E_BADF = 9, E_NOTDIR = 20, E_ISDIR = 21, E_INVAL = 22,
             E_MFILE = 24, E_NAMETOOLONG = 36, E_BUSY = 16 };

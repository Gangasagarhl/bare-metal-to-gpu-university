// ulib.h - the whole "C library" of the OS303 user programs: system-call wrappers and
// three print helpers. A system call puts its number in RAX and arguments in RDI, RSI,
// RDX; the SYSCALL instruction itself overwrites RCX and R11 (F3-29).
#pragma once
#include <stdint.h>

inline int64_t sys3(uint64_t nr, uint64_t a, uint64_t b, uint64_t c)
{
    int64_t r;
    asm volatile("syscall" : "=a"(r) : "a"(nr), "D"(a), "S"(b), "d"(c) : "rcx", "r11", "memory");
    return r;
}
[[noreturn]] inline void sys_exit(int status)
{
    sys3(0, uint64_t(status), 0, 0);
    for (;;) {
    }
}
inline int64_t sys_write(int fd, const void* buf, uint64_t len)
{
    return sys3(1, uint64_t(fd), reinterpret_cast<uint64_t>(buf), len);
}
inline int64_t sys_getpid() { return sys3(2, 0, 0, 0); }
inline int64_t sys_spawn(const char* path, const char* const* argv)
{
    return sys3(3, reinterpret_cast<uint64_t>(path), reinterpret_cast<uint64_t>(argv), 0);
}
inline int64_t sys_wait(int* status) { return sys3(4, reinterpret_cast<uint64_t>(status), 0, 0); }

inline uint64_t ulen(const char* s)
{
    uint64_t n = 0;
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}
inline void print(const char* s) { sys_write(1, s, ulen(s)); }
inline void print_num(int64_t v)   // decimal, with sign
{
    char buf[24];
    int i = 23;
    buf[i] = '\0';
    bool neg = v < 0;
    uint64_t u = neg ? uint64_t(-v) : uint64_t(v);
    do {
        buf[--i] = char('0' + u % 10);
        u /= 10;
    } while (u != 0);
    if (neg) {
        buf[--i] = '-';
    }
    print(&buf[i]);
}
inline void print_hex(uint64_t v)
{
    char buf[19] = "0x";
    for (int i = 0; i < 16; ++i) {
        buf[2 + i] = "0123456789abcdef"[(v >> (60 - 4 * i)) & 15];
    }
    buf[18] = '\0';
    print(buf);
}

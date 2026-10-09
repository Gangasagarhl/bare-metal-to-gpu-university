/* libc.c - F3-54 myos sysroot: the teaching C library. Its system-call layer uses the
   Linux-compatible ABI (F3-50, path 2: numbers 1 = write, 60 = exit on x86-64). */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static long sys3(long nr, long a, long b, long c)
{
    long ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(nr), "D"(a), "S"(b), "d"(c) : "rcx", "r11", "memory");
    return ret;
}

size_t strlen(const char* s)
{
    size_t n = 0;
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

ssize_t write(int fd, const void* buf, size_t n)
{
    return sys3(1, fd, (long)buf, (long)n);
}

_Noreturn void _exit(int status)
{
    for (;;) {
        sys3(60, status, 0, 0);
    }
}

_Noreturn void exit(int status)
{
    _exit(status);
}

int puts(const char* s)
{
    if (write(1, s, strlen(s)) < 0 || write(1, "\n", 1) < 0) {
        return -1;
    }
    return 1;
}

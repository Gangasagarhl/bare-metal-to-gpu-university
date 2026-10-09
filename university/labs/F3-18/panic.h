// panic.h - F3-18: stop the kernel with a message that names the file and line.
#pragma once

[[noreturn]] void panic_at(const char* file, int line, const char* fmt, ...)
    __attribute__((format(printf, 3, 4)));

#define PANIC(...) panic_at(__FILE__, __LINE__, __VA_ARGS__)
#define KASSERT(cond)                                                   \
    do {                                                                \
        if (!(cond)) {                                                  \
            panic_at(__FILE__, __LINE__, "assertion failed: %s", #cond); \
        }                                                               \
    } while (0)

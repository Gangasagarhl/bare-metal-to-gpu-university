/* stdio.h - tlibc, the OS304 teaching C library (a tiny subset: output to standard output only). */
#ifndef TLIBC_STDIO_H
#define TLIBC_STDIO_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
int printf(const char* fmt, ...);   /* %d %u %x %s %c %%; flags - and 0; width; l */
int puts(const char* s);
int putchar(int c);
#ifdef __cplusplus
}
#endif
#endif

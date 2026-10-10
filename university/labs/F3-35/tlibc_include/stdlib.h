/* stdlib.h - tlibc, the OS304 teaching C library (a tiny subset). */
#ifndef TLIBC_STDLIB_H
#define TLIBC_STDLIB_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
[[noreturn]] void exit(int status);
#else
_Noreturn void exit(int status);
#endif
int abs(int v);
#ifdef __cplusplus
}
#endif
#endif

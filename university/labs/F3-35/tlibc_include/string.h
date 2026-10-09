/* string.h - tlibc, the OS304 teaching C library (a tiny subset). */
#ifndef TLIBC_STRING_H
#define TLIBC_STRING_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
size_t strlen(const char* s);
int strcmp(const char* a, const char* b);
void* memcpy(void* d, const void* s, size_t n);
void* memset(void* d, int c, size_t n);
#ifdef __cplusplus
}
#endif
#endif

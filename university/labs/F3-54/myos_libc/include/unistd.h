/* unistd.h - F3-54 myos sysroot (teaching C library) */
#ifndef MYOS_UNISTD_H
#define MYOS_UNISTD_H
#include <stddef.h>
ssize_t write(int fd, const void* buf, size_t n);
_Noreturn void _exit(int status);
#endif

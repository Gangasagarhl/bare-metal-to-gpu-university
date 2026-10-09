/* hello_myos.c - F3-54: the first program built by the myos toolchain against the myos sysroot. */
#include <stdio.h>

int main(int argc, char** argv)
{
    (void)argv;
#if defined(__myos__)
    puts("built for myos: __myos__ is defined");
#endif
#if defined(__linux__)
    puts("__linux__ is defined: host headers or a host triple leaked in");
#else
    puts("__linux__ is not defined: no host contamination");
#endif
    return argc == 1 ? 0 : 1;
}
